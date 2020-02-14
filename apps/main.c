#include <unistd.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <stdio.h>
#include <wait.h>
#include <string.h>
#include <errno.h>
#include <stdnoreturn.h>
#include "worker.h"

static const long DEFAULT_REQUEUE_INTERVAL = 5;
static const char DEFAULT_LOCATION[] = ".";
static const char REQUEUE_PROCESS[] = "requeue";

noreturn void run_requeue(worker_t *worker, long requeue_interval) {
  long wait_seconds = requeue_interval / 2;
  if (wait_seconds < 1) {
    wait_seconds = 1;
  }
  for (;;) {
    int ret = worker_requeue_check(worker, requeue_interval);
    if (ret != 0) {
      fprintf(stderr, "retask: requeue failed with code %i\n", ret);
    }
    sleep(wait_seconds);
  }
}

int run_worker(worker_t *worker) {
  int ret;
  int listener;
  ret = worker_create_listener(worker, &listener);
  if (ret != 0) {
    return ret;
  }
  for (;;) {
    ret = worker_run_all(worker);
    if (ret != 0) {
      break;
    }
    ret = worker_listen(worker, listener);
    if (ret != 0) {
      break;
    }
  }
  close(listener);
  return 0;
}

int launch(worker_t *worker, long requeue_interval) {
  int ret = worker_prepare(worker);
  if (ret != 0) {
    return ret;
  }

  if (fork() == 0) {
    // sub process to manage requeue process
    size_t name_len = strlen(worker->name);
    char *process_name = (char *) malloc(name_len + 1 + sizeof(REQUEUE_PROCESS));
    memcpy(process_name, worker->name, name_len);
    process_name[name_len] = '/';
    memcpy(&process_name[name_len + 1], REQUEUE_PROCESS, sizeof(REQUEUE_PROCESS));
    prctl(PR_SET_NAME, process_name, 0, 0, 0);
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    free(process_name);
    run_requeue(worker, requeue_interval);
    _exit(EXIT_FAILURE);
  }

  return run_worker(worker);
}

void usage() {
  fprintf(stderr, "Executor for tasks from filesystem\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Author: Baryshnikov Aleksandr <owner@reddec.net>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "retask [flags]\n");
  fprintf(stderr, "    -c <directory> [default=.]  use specified directory as work dir\n");
  fprintf(stderr, "    -r <seconds>   [default=%li]  interval for requeue\n", DEFAULT_REQUEUE_INTERVAL);
}

int main(int argc, char *argv[]) {
  int opt;
  const char *location = DEFAULT_LOCATION;
  long requeue_sec = DEFAULT_REQUEUE_INTERVAL;

  while ((opt = getopt(argc, argv, "c:r:")) != -1) {
    switch (opt) {
    case 'r': {
      requeue_sec = strtol(optarg, NULL, 10);
      if (errno == EINVAL || errno == ERANGE) {
        fprintf(stderr, "invalid value for requeue interval: %s\n", optarg);
        usage();
        exit(EXIT_FAILURE);
      }
      break;
    }
    case 'c': {
      location = optarg;
      break;
    }
    default: /* '?' */
      usage();
      exit(EXIT_FAILURE);
    }
  }

  worker_t worker;
  worker_init(&worker, location);
  int ret = launch(&worker, requeue_sec);
  worker_destroy(&worker);
  return ret;
}


