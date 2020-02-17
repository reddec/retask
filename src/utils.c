//
// Created by baryshnikov on 10/02/2020.
//
#include "utils.h"
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

char *find_executable(const char *bin_dir) {
  DIR *dirp;
  struct dirent *direntp;
  dirp = opendir(bin_dir);
  if (dirp == NULL) {
    fprintf(stderr, "can't detect executable in %s: %s\n", bin_dir, strerror(errno));
    return NULL;
  }
  for (;;) {
    direntp = readdir(dirp);
    if (direntp == NULL) {
      break;
    }
    if (direntp->d_type == DT_DIR) {
      continue;
    }
    char *path = filepath_join(bin_dir, direntp->d_name);
    if (!path) {
      fprintf(stderr, "retask: failed find executable in %s (allocate file path): %s\n", bin_dir, strerror(errno));
      return NULL;
    }
    if (access(path, X_OK) != -1) {
      closedir(dirp);
      return path;
    }
    free(path);
  }
  closedir(dirp);
  fprintf(stderr, "there is no executable file in %s\n", bin_dir);
  return NULL;
}

char *filepath_join(const char *root_dir, const char *child) {
  if (!root_dir) {
    return NULL;
  }
  if (!child) {
    return strdup(root_dir);
  }
  size_t child_len = strlen(child);
  size_t root_len = strlen(root_dir);
  size_t n = root_len + child_len + 1;
  char need_slash = 0;
  if (root_len > 0 && root_dir[root_len - 1] != '/') {
    ++n;
    need_slash = 1;
  }
  char *data = malloc(n);
  if (!data) {
    return NULL;
  }
  memcpy(data, root_dir, root_len);
  if (need_slash) {
    data[root_len] = '/';
  }
  memcpy(&data[root_len + need_slash], child, child_len);
  data[n - 1] = '\0';
  return data;
}

const char *filepath_basename(const char *root_dir) {
  size_t len = strlen(root_dir);
  if (len == 0) {
    return root_dir;
  }
  for (ssize_t i = len - 1; i >= 0; --i) {
    if (root_dir[i] == '/') {
      return &root_dir[i + 1];
    }
  }
  return root_dir;
}
