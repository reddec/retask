//
// Created by baryshnikov on 11/02/2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include "../include/utils.h"
#include "../include/task.h"

int __run_app(const task_t *task, int stdin_file, int stdout_file);

int task_init(task_t *task, const worker_t *worker, const char *task_name) {
  char *executable = find_executable(worker->bin_dir);
  if (!executable) {
    return -1;
  }
  task->name = strdup(task_name);
  task->file = filepath_join(worker->tasks_dir, task_name);
  task->progress_file = filepath_join(worker->progress_dir, task_name);
  task->result_file = filepath_join(worker->complete_dir, task_name);
  task->requeue_file = filepath_join(worker->requeue_dir, task_name);
  task->executable = executable;
  task->worker = worker;
  return 0;
}

void task_destroy(task_t *task) {
  free(task->file);
  free(task->progress_file);
  free(task->result_file);
  free(task->requeue_file);
  free(task->executable);
  free(task->name);
}

int task_run_app(const task_t *task) {
  FILE *progress = fopen(task->progress_file, "w");
  if (!progress) {
    fprintf(stderr, "retask: create progress file %s: %s", task->progress_file, strerror(errno));
    return 1;
  }
  FILE *source = fopen(task->file, "r");
  if (!source) {
    fprintf(stderr, "retask: read task file %s: %s", task->file, strerror(errno));
    fclose(progress);
    return 2;
  }

  int ret = __run_app(task, fileno(source), fileno(progress));
  fclose(source);
  fclose(progress);
  return ret;
}

int __run_app(const task_t *task, int stdin_file, int stdout_file) {
  pid_t pid = 0;
  pid = fork();
  if (pid == 0) {
    dup2(stdin_file, STDIN_FILENO);
    dup2(stdout_file, STDOUT_FILENO);
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    setenv("WORKER_ID", task->worker->name, 1);
    setenv("WORKER_ROOT_DIR", task->worker->root, 1);
    setenv("WORKER_BIN_DIR", task->worker->bin_dir, 1);
    setenv("WORKER_TASKS_DIR", task->worker->tasks_dir, 1);
    setenv("TASK_ID", task->name, 1);
    setenv("TASK_EXECUTABLE", task->executable, 1);

    execl(task->executable, task->executable, (char *) NULL);
    _exit(1);
  }
  int status;
  if (waitpid(pid, &status, 0) == -1) {
    perror("waitpid() failed");
    return 1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return status;
}

int task_complete(const task_t *task, int ret_code) {
  if (ret_code != 0) {
    remove(task->progress_file);
    return rename(task->file, task->requeue_file);
  }
  ret_code = rename(task->progress_file, task->result_file);
  if (ret_code != 0) {
    return ret_code;
  }
  return remove(task->file);
}

int task_run(const task_t *task) {
  int ret_code = task_run_app(task);
  ret_code = task_complete(task, ret_code);

  return ret_code;
}

