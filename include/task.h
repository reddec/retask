//
// Created by baryshnikov on 11/02/2020.
//

#ifndef RETASK_TASK_H
#define RETASK_TASK_H

#include "worker.h"

/**
 * Definition of single task inside worker. Worker should not be destroyed before task.
 */
typedef struct task_t {
  char *name;             // task name (basename of task file)
  char *file;             // task file. The file will be used as STDIN for the executable
  char *progress_file;    // temp file for STDOUT of the executable
  char *result_file;      // destination of progress file in case of successful execution (ret code 0)
  char *requeue_file;     // destination for task file in case of failed execution (for re-queue)
  char *executable;       // executable (first file marked as executable inside bin dir)
  const worker_t *worker; // reference to parent worker instance
} task_t;

/**
 * Initialize internal resource for task, allocate paths, detect executable in a bin dir.
 * Paths will be allocated by malloc.
 * @param task reference to task that will be initialized
 * @param worker initialized instance of worker
 * @param task_name task name (basename of file in tasks dir)
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int task_init(task_t *task, const worker_t *worker, const char *task_name);

/**
 * Destroy (free) allocated resource (worker ref will not be touched)
 * @param task initialized instance of task
 */
void task_destroy(task_t *task);

/**
 * Run executable and process result files (re-queue or move to complete). This function internally runs fork and fill
 * following environment variables: WORKER_ID, WORKER_ROOT_DIR, WORKER_BIN_DIR, WORKER_TASKS_DIR, TASK_ID, TASK_EXECUTABLE.
 * Task file will be used as STDIN and progress file as STDOUT. STDERR will be mapped as parent process.
 * @param task initialized instance of task
 * @return  0 on success, otherwise print error to stderr and return related code
 */
int task_run(const task_t *task);

#endif //RETASK_TASK_H
