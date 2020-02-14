//
// Created by baryshnikov on 10/02/2020.
//

#ifndef RETASK_UTILS_H
#define RETASK_UTILS_H
/**
 * Concat root dir and child with respect of root dir ending (with / or without).
 * It is caller responsible to free returned memory.
 * @param root_dir parent directory
 * @param child child directory
 * @return joined path
 */
char *filepath_join(const char *root_dir, const char *child);

/**
 * Find first file with executable flag.
 * It is caller responsible to free returned memory.
 * @param bin_dir directory to scan
 * @return full path on success, otherwise print error to stderr and return NULL
 */
char *find_executable(const char *bin_dir);

/**
 * Get basename (last part after /). There is no memory allocation here.
 * @param root_dir path to scan
 * @return ref to the first symbol in root_dir var
 */
const char *filepath_basename(const char *root_dir);
#endif //RETASK_UTILS_H
