//
// Created by baryshnikov on 10/02/2020.
//

#ifndef RETASK_WORKER_H
#define RETASK_WORKER_H

/**
 * Definition of worker in a directory.
 * Single directory should be handled by single worker
 */
typedef struct worker_t {
  char *root;         // root directory
  char *name;         // worker ID/name - base name of root directory
  char *tasks_dir;    // directory with files that will be fed to the application as STDIN (tasks)
  char *requeue_dir;  // directory where failed tasks will be added for later re-processing
  char *progress_dir; // directory where stdout of application will be stored and used as result (on success)
  char *complete_dir; // directory where stdout of successful tasks execution will be stored
  char *bin_dir;      // directory where application should be stored (first executable file)
} worker_t;

/**
 * Initialize worker. Function uses malloc internally to allocate and set worker structure.
 * Allocated worker should be destroyed by `worker_destroy`
 * @param worker pointer to worker
 * @param root_dir root directory that will be used as parent for all nested dirs (tasks, progress, ..)
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int worker_init(worker_t *worker, const char *root_dir);

/**
 * Free all allocated resource in worker
 * @param worker pre-initialized by `worker_init` instance
 */
void worker_destroy(worker_t *worker);

/**
 * Prepare environment for worker: creates directories (however, root directory should be already created),
 * clean old files and so on.
 * @param worker initialized instance of worker
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int worker_prepare(const worker_t *worker);

/**
 * Run all queue tasks: sequentially execute `worker_run_one` for each file in tasks dir, sorted alphabetically.
 * Fail-tolerant: print error if tasks failed but do not stop next executions.
 * Mostly error could return if no executable found in bin-dir
 * @param worker  initialized instance of worker
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int worker_run_all(const worker_t *worker);

/**
 * Execute only one task identified by name (base name of task file in tasks dir).
 * In case of failure - task will be automatically re-queued, in case of successful (return code 0) execution -
 * stdout will be stored in complete-dir with same name as task.
 * @param worker initialized instance of worker
 * @param task_name name of task  (base name of task file in tasks dir)
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int worker_run_one(const worker_t *worker, const char *task_name);

/**
 * Initialize inotify subsystem to watch tasks directory.
 * @param worker initialized instance of worker
 * @param listener reference to where inotify file descriptor will be put
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int worker_create_listener(const worker_t *worker, int *listener);

/**
 * Wait or inotify events
 * @param worker initialized instance of worker
 * @param listener inotify file descriptor (commonly from `worker_create_listener`)
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int worker_listen(const worker_t *worker, int listener);

/**
 * Check files in re-queue directory for expired tasks and move them back to tasks dir
 * @param worker initialized instance of worker
 * @param expiration_sec expiration time in seconds
 * @return 0 on success, otherwise print error to stderr and return related code
 */
int worker_requeue_check(const worker_t *worker, long expiration_sec);

#endif //RETASK_WORKER_H
