#include <stdbool.h>

#include "mgos.h"

static FILE *s_curfile = NULL;
static char *s_curfilename = NULL;

/*
 * Writes filenames of the oldest and/or newest log files to the provided
 * pointers. Pointers can be NULL.
 *
 * Returns total number of log files found.
 */
static int get_oldest_newest(char **poldest, char **pnewest) {
  int cnt = 0;

  if (poldest != NULL) {
    *poldest = NULL;
  }
  if (pnewest != NULL) {
    *pnewest = NULL;
  }

  const char *prefix = mgos_sys_config_get_file_logger_prefix();
  DIR *dirp;
  if ((dirp = (opendir(mgos_sys_config_get_file_logger_dir()))) != NULL) {
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL) {
      if (strncmp(dp->d_name, prefix, strlen(prefix)) == 0) {
        /* One of the log files */
        cnt++;

        if (poldest != NULL) {
          if (*poldest == NULL || strcmp(dp->d_name, *poldest) < 0) {
            free(*poldest);
            *poldest = strdup(dp->d_name);
          }
        }

        if (pnewest != NULL) {
          if (*pnewest == NULL || strcmp(dp->d_name, *pnewest) > 0) {
            free(*pnewest);
            *pnewest = strdup(dp->d_name);
          }
        }
      }
    }
    closedir(dirp);
  }

  return cnt;
}

/*
 * Allocates and returns new filename for the log; caller needs to free it.
 */
static char *get_new_log_filename(void) {
  struct mg_str logsdir = mg_mk_str(mgos_sys_config_get_file_logger_dir());
  char *ret = NULL;

  if (logsdir.len > 0 && logsdir.p[logsdir.len - 1] == '/') {
    logsdir.len--;
  }

  double td = mg_time();
  time_t t = (time_t) td;
  struct tm tm;
#ifdef _REENT
  localtime_r(&t, &tm);
#else
  memcpy(&tm, localtime(&t), sizeof(tm));
#endif
  mg_asprintf(&ret, 0, "%.*s/%s%.4d%.2d%.2d-%.2d%.2d%.2d.%.3d.txt", logsdir.len,
              logsdir.p, mgos_sys_config_get_file_logger_prefix(),
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
              tm.tm_min, tm.tm_sec, (int) ((td - (long long) (td)) * 1000));
  return ret;
}

static void init_curfile(void) {
  if (s_curfile != NULL) {
    fclose(s_curfile);
    s_curfile = NULL;
  }
  s_curfile = fopen(s_curfilename, "a");
  if (s_curfile == NULL) {
    LOG(LL_ERROR, ("failed to open log file '%s'", s_curfilename));
    return;
  }
  setvbuf(s_curfile, NULL, _IOLBF, 256);
}

static void debug_write_cb(int ev, void *ev_data, void *userdata) {
  const struct mgos_debug_hook_arg *arg =
      (const struct mgos_debug_hook_arg *) ev_data;

  if (s_curfile == NULL) {
    /*
     * It could happen if only there was an issue with opening a file:
     * just do nothing then.
     */
    return;
  }

  /* Before writing to the current log file, check if it's too large already */
  if (ftell(s_curfile) >= mgos_sys_config_get_file_logger_max_file_size()) {
    /* Current file is too large, need to create a new one */
    free(s_curfilename);
    s_curfilename = get_new_log_filename();
    init_curfile();

    /* Also need to check if there are too many files */
    char *oldest = NULL;
    int log_files_cnt;
    do {
      log_files_cnt = get_oldest_newest(&oldest, NULL);

      if (log_files_cnt > mgos_sys_config_get_file_logger_max_num_files()) {
        /* Yes, there are too many; delete the found oldest one */
        remove(oldest);
        log_files_cnt--;
      }

      free(oldest);
      oldest = NULL;
    } while (log_files_cnt > mgos_sys_config_get_file_logger_max_num_files());
  }

  /* Finally, write piece of data to the current log file */
  if (s_curfile != NULL) {
    fwrite(arg->data, arg->len, 1, s_curfile);
  }

  (void) ev;
  (void) userdata;
}

bool mgos_file_logger_init(void) {
  if (!mgos_sys_config_get_file_logger_enable()) return true;

  /* Get the newest filename (if any) */
  get_oldest_newest(NULL, &s_curfilename);

  if (s_curfilename == NULL) {
    /* No log files are found, generate a new one */
    s_curfilename = get_new_log_filename();
  }

  init_curfile();

  mgos_event_add_handler(MGOS_EVENT_LOG, debug_write_cb, NULL);

  return true;
}
