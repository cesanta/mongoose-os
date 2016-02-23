/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

#include "common/cs_dirent.h"

/*
 * This file contains POSIX opendir/closedir/readdir API implementation
 * for systems which do not natively support it (e.g. Windows).
 */

#ifndef MG_FREE
#define MG_FREE free
#endif

#ifndef MG_MALLOC
#define MG_MALLOC malloc
#endif

#ifdef _WIN32
DIR *opendir(const char *name) {
  DIR *dir = NULL;
  wchar_t wpath[MAX_PATH];
  DWORD attrs;

  if (name == NULL) {
    SetLastError(ERROR_BAD_ARGUMENTS);
  } else if ((dir = (DIR *) MG_MALLOC(sizeof(*dir))) == NULL) {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  } else {
    to_wchar(name, wpath, ARRAY_SIZE(wpath));
    attrs = GetFileAttributesW(wpath);
    if (attrs != 0xFFFFFFFF && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
      (void) wcscat(wpath, L"\\*");
      dir->handle = FindFirstFileW(wpath, &dir->info);
      dir->result.d_name[0] = '\0';
    } else {
      MG_FREE(dir);
      dir = NULL;
    }
  }

  return dir;
}

int closedir(DIR *dir) {
  int result = 0;

  if (dir != NULL) {
    if (dir->handle != INVALID_HANDLE_VALUE)
      result = FindClose(dir->handle) ? 0 : -1;
    MG_FREE(dir);
  } else {
    result = -1;
    SetLastError(ERROR_BAD_ARGUMENTS);
  }

  return result;
}

struct dirent *readdir(DIR *dir) {
  struct dirent *result = NULL;

  if (dir) {
    if (dir->handle != INVALID_HANDLE_VALUE) {
      result = &dir->result;
      (void) WideCharToMultiByte(CP_UTF8, 0, dir->info.cFileName, -1,
                                 result->d_name, sizeof(result->d_name), NULL,
                                 NULL);

      if (!FindNextFileW(dir->handle, &dir->info)) {
        (void) FindClose(dir->handle);
        dir->handle = INVALID_HANDLE_VALUE;
      }

    } else {
      SetLastError(ERROR_FILE_NOT_FOUND);
    }
  } else {
    SetLastError(ERROR_BAD_ARGUMENTS);
  }

  return result;
}
#endif

#ifdef CS_ENABLE_SPIFFS

DIR *opendir(const char *dir_name) {
  DIR *dir = NULL;
  extern spiffs fs;

  if (dir_name != NULL && (dir = (DIR *) malloc(sizeof(*dir))) != NULL &&
      SPIFFS_opendir(&fs, (char *) dir_name, &dir->dh) == NULL) {
    free(dir);
    dir = NULL;
  }

  return dir;
}

int closedir(DIR *dir) {
  if (dir != NULL) {
    SPIFFS_closedir(&dir->dh);
    free(dir);
  }
  return 0;
}

struct dirent *readdir(DIR *dir) {
  return SPIFFS_readdir(&dir->dh, &dir->de);
}

/* SPIFFs doesn't support directory operations */
int rmdir(const char *path) {
  (void) path;
  return ENOTDIR;
}

int mkdir(const char *path, mode_t mode) {
  (void) path;
  (void) mode;
  /* for spiffs supports only root dir, which comes from mongoose as '.' */
  return (strlen(path) == 1 && *path == '.') ? 0 : ENOTDIR;
}

#endif /* CS_ENABLE_SPIFFS */

#endif /* EXCLUDE_COMMON */

/* ISO C requires a translation unit to contain at least one declaration */
typedef int cs_dirent_dummy;
