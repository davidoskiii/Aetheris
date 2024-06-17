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
      int len = editor.row[filerow].size - editor.coloff;
      if (len < 0) len = 0;
      if (len > editor.screencols) len = editor.screencols;
      abAppend(ab, &editor.row[filerow].chars[editor.coloff], len);
    }

    abAppend(ab, "\x1b[K", 3);
    if (y < editor.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void editorAppendRow(char *s, size_t len) {
  editor.row = realloc(editor.row, sizeof(erow) * (editor.numrows + 1));

  int at = editor.numrows;
  editor.row[at].size = len;
  editor.row[at].chars = malloc(len + 1);
  memcpy(editor.row[at].chars, s, len);
  editor.row[at].chars[len] = '\0';
  editor.numrows++;
}

void editorRefreshScreen() {
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (editor.cy - editor.rowoff) + 1, (editor.cx - editor.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorScroll() {
  if (editor.cy < editor.rowoff) {
    editor.rowoff = editor.cy;
  }

  if (editor.cy >= editor.rowoff + editor.screenrows) {
    editor.rowoff = editor.cy - editor.screenrows + 1;
  }

  if (editor.cx < editor.coloff) {
    editor.coloff = editor.cx;
  }

  if (editor.cx >= editor.coloff + editor.screencols) {
    editor.coloff = editor.cx - editor.screencols + 1;
  }
}
