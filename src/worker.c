//
// Created by baryshnikov on 10/02/2020.
//

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "task.h"
#include "utils.h"

void worker_init(worker_t *worker, const char *root_dir) {
  char *bin_dir = filepath_join(root_dir, "app");
  worker->root = strdup(root_dir);
  worker->name = strdup(filepath_basename(root_dir));
  worker->bin_dir = bin_dir;
  worker->progress_dir = filepath_join(worker->root, "progress");
  worker->requeue_dir = filepath_join(worker->root, "requeue");
  worker->tasks_dir = filepath_join(worker->root, "tasks");
  worker->complete_dir = filepath_join(worker->root, "complete");
}

void worker_destroy(worker_t *worker) {
  if (!worker) {
    return;
  }
  free(worker->root);
  free(worker->name);
  free(worker->progress_dir);
  free(worker->requeue_dir);
  free(worker->tasks_dir);
  free(worker->complete_dir);
  free(worker->bin_dir);
}

static const mode_t dir_mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

int worker_create_dirs(const worker_t *worker) {
  int ret;
  ret = mkdir(worker->tasks_dir, dir_mode);
  if (ret != 0 && errno != EEXIST) {
    perror("retask: create task dir");
    return ret;
  }
  ret = mkdir(worker->progress_dir, dir_mode);
  if (ret != 0 && errno != EEXIST) {
    perror("retask: create progress dir");
    return ret;
  }
  ret = mkdir(worker->requeue_dir, dir_mode);
  if (ret != 0 && errno != EEXIST) {
    perror("retask: create requeue dir");
    return ret;
  }
  ret = mkdir(worker->complete_dir, dir_mode);
  if (ret != 0 && errno != EEXIST) {
    perror("retask: create complete dir");
    return ret;
  }
  ret = mkdir(worker->bin_dir, dir_mode);
  if (ret != 0 && errno != EEXIST) {
    perror("retask: create bin dir");
    return ret;
  }
  return 0;
}

int worker_clean_progress(const worker_t *worker) {
  DIR *dp;
  struct dirent *ep;
  dp = opendir(worker->progress_dir);
  if (!dp) {
    fprintf(stderr, "retask: open progress dir %s: %s\n", worker->progress_dir, strerror(errno));
    return errno;
  }
  while ((ep = readdir(dp))) {
    if (ep->d_type == DT_DIR) {
      continue;
    }
    char *path = filepath_join(worker->progress_dir, ep->d_name);
    int ret = remove(path);
    if (ret != 0) {
      fprintf(stderr, "retask: remove progress file %s: %s\n", path, strerror(errno));
    }
    free(path);
  }
  closedir(dp);
  return 0;
}

int worker_prepare(const worker_t *worker) {
  int ret;
  ret = worker_create_dirs(worker);
  if (ret != 0) {
    return ret;
  }
  ret = worker_clean_progress(worker);
  if (ret != 0) {
    return ret;
  }
  return 0;
}

int worker_run_all(const worker_t *worker) {
  struct dirent **namelist;

  int n = scandir(worker->tasks_dir, &namelist, NULL, alphasort);
  if (n == -1) {
    perror("retask: scandir for tasks");
    return -1;
  }

  for (int i = 0; i < n; ++i) {
    struct dirent *entry = namelist[i];
    if (entry->d_type == DT_DIR) {
      continue;
    }
    int ret = worker_run_one(worker, entry->d_name);
    if (ret != 0) {
      fprintf(stderr, "retask: task %s failed with code %i\n", entry->d_name, ret);
    }
    free(entry);
  }
  free(namelist);
  return 0;
}

int worker_run_one(const worker_t *worker, const char *task_name) {
  task_t task;
  int ret = task_init(&task, worker, task_name);
  if (ret != 0) {
    return ret;
  }
  ret = task_run(&task);
  task_destroy(&task);
  fprintf(stderr, "retask: task %s executed with code %i\n", task_name, ret);
  return ret;
}

int worker_create_listener(const worker_t *worker, int *listener) {
  int fd = inotify_init();
  if (fd == -1) {
    fprintf(stderr, "retask: failed to add watcher on directory %s: %s\n", worker->tasks_dir, strerror(errno));
    return -1;
  }
  int rc = inotify_add_watch(fd, worker->tasks_dir, IN_MOVED_TO | IN_CLOSE_WRITE);
  if (rc == -1) {
    fprintf(stderr, "retask: failed to add watcher on directory %s: %s\n", worker->tasks_dir, strerror(errno));
    close(fd);
    return -1;
  }
  *listener = fd;
  return 0;
}

int worker_listen(const worker_t *worker, int listener) {
  const struct inotify_event *event;
  char *ptr;
  char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
  ssize_t len = read(listener, buf, sizeof(buf));
  if (len == -1 && errno != EAGAIN) {
    fprintf(stderr, "retask: failed to listen events on directory %s: %s\n", worker->tasks_dir, strerror(errno));
    return -1;
  }
  for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
    event = (const struct inotify_event *) ptr;
    if (event->len)
      fprintf(stderr, "retask: detected %s", event->name);
    if (event->mask & IN_ISDIR)
      fprintf(stderr, " [directory]\n");
    else
      fprintf(stderr, " [file]\n");
  }

  return 0;
}

int worker_requeue_check(const worker_t *worker, __time_t expiration_sec) {
  DIR *dp;
  struct dirent *ep;
  struct stat info;
  struct timeval now;
  int ret = gettimeofday(&now, NULL);
  if (ret != 0) {
    fprintf(stderr, "retask: get current date/time: %s\n", strerror(errno));
    return ret;
  }
  dp = opendir(worker->requeue_dir);
  if (!dp) {
    ret = errno;
    fprintf(stderr, "retask: open requeue dir %s: %s\n", worker->requeue_dir, strerror(errno));
    return ret;
  }
  while ((ep = readdir(dp))) {
    if (ep->d_type == DT_DIR) {
      continue;
    }
    char *src = filepath_join(worker->requeue_dir, ep->d_name);

    ret = stat(src, &info);
    if (ret != 0) {
      fprintf(stderr, "retask: stat queued file %s: %s\n", src, strerror(errno));
      free(src);
      continue;
    }
    if (info.st_ctim.tv_sec + expiration_sec > now.tv_sec) {
      continue;
    }
    char *dst = filepath_join(worker->tasks_dir, ep->d_name);
    ret = rename(src, dst);
    if (ret != 0) {
      fprintf(stderr, "retask: failed move queued file %s to tasks %s: %s\n", src, dst, strerror(errno));
    } else {
      fprintf(stderr, "retask: re-queued file %s due to expiration time\n", src);
    }
    free(dst);
    free(src);
  }
  closedir(dp);
  return 0;
}
