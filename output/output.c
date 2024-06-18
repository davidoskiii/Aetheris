#include <termios.h>
#include <string.h>
#include <errno.h>

#include "output.h"

#include "../terminal/terminal.h"
#include "../utils/utils.h"
#include "../common.h"

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

void editorDrawRows(struct abuf *ab) {
  int y;

  for (y = 0; y < editor.screenrows; y++) {
    int filerow = y + editor.rowoff;
    if (filerow >= editor.numrows) {
      if (editor.numrows == 0 && y == editor.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Aetheris Text Editor -- version %s", AETHERIS_VERSION);
        if (welcomelen > editor.screencols) welcomelen = editor.screencols;
        int padding = (editor.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = editor.row[filerow].rsize - editor.coloff;
      if (len < 0) len = 0;
      if (len > editor.screencols) len = editor.screencols;
      abAppend(ab, &editor.row[filerow].render[editor.coloff], len);
    }

    abAppend(ab, "\x1b[K", 3);
    if (y < editor.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void editorUpdateRow(erow *row) {
  int tabs = 0;
  int j;

  for (j = 0; j < row->size; j++) if (row->chars[j] == '\t') tabs++;

  free(row->render);
  row->render = malloc(row->size + tabs*7 + 1);

  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % 8 != 0) row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }

  row->render[idx] = '\0';
  row->rsize = idx;
}

void editorAppendRow(char *s, size_t len) {
  editor.row = realloc(editor.row, sizeof(erow) * (editor.numrows + 1));

  int at = editor.numrows;
  editor.row[at].size = len;
  editor.row[at].chars = malloc(len + 1);
  memcpy(editor.row[at].chars, s, len);
  editor.row[at].chars[len] = '\0';

  editor.row[at].rsize = 0;
  editor.row[at].render = NULL;
  editorUpdateRow(&editor.row[at]);

  editor.numrows++;
}

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (AETHERIS_TAB_STOP - 1) - (rx % AETHERIS_TAB_STOP);
    rx++;
  }
  return rx;
}

void editorRefreshScreen() {
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (editor.cy - editor.rowoff) + 1, (editor.rx - editor.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorScroll() {
  editor.rx = 0;

  if (editor.cy < editor.numrows) {
    editor.rx = editorRowCxToRx(&editor.row[editor.cy], editor.cx);
  }

  if (editor.cy < editor.rowoff) {
    editor.rowoff = editor.cy;
  }

  if (editor.cy >= editor.rowoff + editor.screenrows) {
    editor.rowoff = editor.cy - editor.screenrows + 1;
  }

  if (editor.rx < editor.coloff) {
    editor.coloff = editor.rx;
  }

  if (editor.rx >= editor.coloff + editor.screencols) {
    editor.coloff = editor.rx - editor.screencols + 1;
  }
}
