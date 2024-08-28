#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#include "../output/output.h"
#include "../syntax/syntax.h"
#include "../input/input.h"
#include "../common.h"

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

int is_integer(const char *str) {
  if (str == NULL) {
    return 0;
  }
  while (isspace((unsigned char)*str)) {
    str++;
  }
  if (*str == '\0') {
    return 0;
  }
  if (*str == '+' || *str == '-') {
    str++;
  }
  if (!isdigit((unsigned char)*str)) {
    return 0;
  }
  char *endptr;
  errno = 0;
  long value = strtol(str, &endptr, 10);
  if (errno != 0 || *endptr != '\0') {
    return 0;
  }
  if (value > (long)INT_MAX || value < (long)INT_MIN) {
    return 0;
  }
  return 1;
}

void editorOpen(char *filename) {
  free(editor.filename);
  editor.filename = strdup(filename);

  editorSelectSyntaxHighlight();

  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
    editorInsertRow(editor.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  editor.dirty = 0;
}

void editorQuit() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  exit(0);
}

void editorFindCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;

  static int saved_hl_line;
  static char *saved_hl = NULL;
  if (saved_hl) {
    memcpy(editor.row[saved_hl_line].hl, saved_hl, editor.row[saved_hl_line].rsize);
    free(saved_hl);
    saved_hl = NULL;
  }

  if (key == '\r' || key == '\x1b') {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }

  if (last_match == -1) direction = 1;
  int current = last_match;
  int i;
  for (i = 0; i < editor.numrows; i++) {
    current += direction;
    if (current == -1) current = editor.numrows - 1;
    else if (current == editor.numrows) current = 0;
    erow *row = &editor.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      last_match = current;
      editor.cy = current;
      editor.cx = editorRowRxToCx(row, match - row->render);
      editor.rowoff = editor.numrows;

      saved_hl_line = current;
      saved_hl = malloc(row->rsize);
      memcpy(saved_hl, row->hl, row->rsize);
      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind() {
  int saved_cx = editor.cx;
  int saved_cy = editor.cy;
  int saved_coloff = editor.coloff;
  int saved_rowoff = editor.rowoff;
  char *query = editorPrompt("Search: %s (Use Esc/Arrows/Enter)", editorFindCallback);
  if (query) {
    free(query);
  } else {
    editor.cx = saved_cx;
    editor.cy = saved_cy;
    editor.coloff = saved_coloff;
    editor.rowoff = saved_rowoff;
  }
}

void editorGotoLine(char* query) {
  if (query == NULL) return;
  int line = atoi(query);
  if (line < 0) {
    line = editor.numrows + 1 + line;
  }

  if (line > editor.numrows) {
    editorSetStatusMessage("%d is bigger than file length (%d lines)", line, editor.numrows);
    return;
  }

  if (line > 0 && line <= editor.numrows) {
    editor.cx = 0;
    editor.sx = 0;
    editor.cy = line - 1;
  }

  if (query) {
    free(query);
  }
}

void editorQuitSafe(int quit_times) {
  if (editor.dirty && quit_times > 0) {
    editorSetStatusMessage("WARNING!!! File has unsaved changes. "
      "Press Ctrl-Q %d more times to quit.", quit_times);
    quit_times--;
    return;
  }
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  exit(0);
}

void editorSave() {
  if (editor.filename == NULL) {
    editor.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
    if (editor.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
    editorSelectSyntaxHighlight();
  }
  int len;
  char *buf = editorRowsToString(&len);
  int fd = open(editor.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        editor.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }
  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < editor.numrows; j++)
    totlen += editor.row[j].size + 1;
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < editor.numrows; j++) {
    memcpy(p, editor.row[j].chars, editor.row[j].size);
    p += editor.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}
