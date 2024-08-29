#include <ctype.h>
#include <termios.h>
#include <string.h>
#include <stdarg.h>

#include "output.h"

#include "../syntax/syntax.h"
#include "../utils/utils.h"

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

void editorDrawStatusBar(struct abuf *ab) {
  editor.screencols += editor.linenum_indent;
  abAppend(ab, "\x1b[7m", 4);
  char status[80], rstatus[80];
  char* mode;
  if (editor.mode == MODE_NORMAL) {
    mode = "NORMAL";
  } else if (editor.mode == MODE_INSERT) {
    mode = "INSERT";
  } else if (editor.mode == MODE_VISUAL) {
    mode = "VISUAL";
  }
  int len = snprintf(status, sizeof(status), " %s | %.20s - %d lines %s", mode, 
        editor.filename ? editor.filename : "[No Name]", 
        editor.numrows, editor.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
    editor.syntax ? editor.syntax->filetype : "no ft", editor.cy + 1, editor.numrows);
  if (len > editor.screencols) len = editor.screencols;
  abAppend(ab, status, len);
  while (len < editor.screencols) {
    if (editor.screencols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
  editor.screencols -= editor.linenum_indent;
}

void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(editor.statusmsg);
  if (msglen > editor.screencols) msglen = editor.screencols;

  int padding = (editor.screencols - msglen) / 2;

  if (padding) padding--;

  while (padding--) abAppend(ab, " ", 1);

  if (msglen && time(NULL) - editor.statusmsg_time < 5) abAppend(ab, editor.statusmsg, msglen);
}

void editorDrawRows(struct abuf *ab) {
  editorSelectText();

  int y;
  for (y = 0; y < editor.screenrows; y++) {
    int filerow = y + editor.rowoff;

    char format[8];
    char linenum[editor.linenum_indent + 1];

    memset(linenum, ' ', editor.linenum_indent);
    snprintf(format, 5, "%%%dd ", editor.linenum_indent - 1);

    if (filerow < editor.numrows) {
      snprintf(linenum, editor.linenum_indent + 1, format, filerow + 1);
    }
    abAppend(ab, linenum, editor.linenum_indent);

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
      char *c = &editor.row[filerow].render[editor.coloff];
      unsigned char *hl = &editor.row[filerow].hl[editor.coloff];
      unsigned char* selected = &(editor.row[filerow].selected[editor.coloff]);
      int current_color = -1;
      int j;
      for (j = 0; j < len; j++) {
        if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abAppend(ab, "\x1b[7m", 4);
          abAppend(ab, &sym, 1);
          abAppend(ab, "\x1b[m", 3);
          if (current_color != -1) {
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
            abAppend(ab, buf, clen);
          }
        } else if (editor.is_selected && selected[j]) {
          current_color = -2;
          abAppend(ab, "\x1b[30;47m", 8);
          abAppend(ab, &c[j], 1);
          abAppend(ab, "\x1b[m", 3);
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            abAppend(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
        }
      }
      abAppend(ab, "\x1b[m", strlen("\xb1[m"));
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
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

  editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > editor.numrows) return;
  editor.row = realloc(editor.row, sizeof(erow) * (editor.numrows + 1));
  memmove(&editor.row[at + 1], &editor.row[at], sizeof(erow) * (editor.numrows - at));
  for (int j = at + 1; j <= editor.numrows; j++) editor.row[j].idx++;

  editor.row[at].idx = at;

  editor.row[at].size = len;
  editor.row[at].chars = malloc(len + 1);
  memcpy(editor.row[at].chars, s, len);
  editor.row[at].chars[len] = '\0';

  editor.row[at].rsize = 0;
  editor.row[at].render = NULL;
  editor.row[at].hl = NULL;
  editor.row[at].selected = NULL;
  editor.row[at].hl_open_comment = 0;
  editorUpdateRow(&editor.row[at]);

  editor.numrows++;
  editor.dirty++;
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

int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (AETHERIS_TAB_STOP - 1) - (cur_rx % AETHERIS_TAB_STOP);
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}

void editorUpdateLinenumIndent() {
  int digit;
  int numrows = editor.numrows;

  if (numrows == 0) {
    digit = 0;
    editor.linenum_indent = 2;
    return;
  }

  digit = 1;
  while (numrows >= 10) {
    numrows = numrows / 10;
    digit++;
  }
  editor.linenum_indent = digit + 2;
}

void editorRefreshScreen() {
  editorUpdateLinenumIndent();
  editor.screencols = editor.raw_screencols - editor.linenum_indent;

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);

  if (editor.mode == MODE_INSERT) {
    // Insert mode: Change to vertical bar
    abAppend(&ab, "\x1b[6 q", 5);
  } else if (editor.mode == MODE_NORMAL) {
    // Normal mode: Change to block cursor
    abAppend(&ab, "\x1b[2 q", 5);
  }

  abAppend(&ab, "\x1b[H", 3);

  editorScroll();

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", editor.cy - editor.rowoff + 1, editor.rx - editor.coloff + 1 + editor.linenum_indent);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(editor.statusmsg, sizeof(editor.statusmsg), fmt, ap);
  va_end(ap);
  editor.statusmsg_time = time(NULL);
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
