#include <ctype.h>
#include <termios.h>
#include <string.h>
#include <stdarg.h>

#include "output.h"

#include "../syntax/syntax.h"
#include "../utils/utils.h"
#include "../config/config.h"

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
  int cols = editor.screencols + editor.numrows_digits + 1;
  char color[20];
  colorToANSI(editor.cfg->status_color[0], color, 0);
  abufAppend(ab, color);
  colorToANSI(editor.cfg->status_color[1], color, 1);
  abufAppend(ab, color);

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

  erow *row = (editor.cy >= editor.numrows) ? NULL : &editor.row[editor.cy];
  int rowlen = row ? row->size : 0;

  int rlen = snprintf(rstatus, sizeof(rstatus), "%s | Line: %d/%d | Col: %d/%d  ",
    editor.syntax ? editor.syntax->filetype : "no ft", editor.cy + 1, editor.numrows, editor.rx + 1, rowlen + 1);
  if (len > cols) len = cols;
  abAppend(ab, status, len);
  while (len < cols) {
    if (cols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abufAppend(ab, ANSI_CLEAR);
}

void editorDrawMessageBar(struct abuf *ab) {
  int cols = editor.screencols + editor.numrows_digits + 1;
  abufAppend(ab, "\x1b[K");
  int msglen = strlen(editor.statusmsg);
  if (msglen > cols) msglen = cols;

  int padding = (cols - msglen) / 2;

  if (padding) padding--;

  while (padding--) abufAppend(ab, " ");

  if (msglen)
      abufAppendN(ab, editor.statusmsg, msglen);

  padding = (cols - msglen) / 2;

  if (padding) padding--;

  while (padding--) abufAppend(ab, " ");

  abufAppend(ab, "\r\n");
}

void editorDrawRows(struct abuf *ab) {
  editorSelectText();

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
          abufAppend(ab, "~");
          padding--;
        }
        while (padding--) abufAppendN(ab, " ", 1);
        abufAppendN(ab, welcome, welcomelen);
      } else {
        abufAppend(ab, "~");
      }
    } else {
      char line_number[16];
      if (filerow == editor.cy) {
        abufAppend(ab, "\x1b[30;100m");
      } else {
        abufAppend(ab, "\x1b[90m");
      }
      snprintf(line_number, sizeof(line_number), "%*d ",
             editor.numrows_digits, filerow + 1);
      abufAppend(ab, line_number);
      abufAppend(ab, ANSI_CLEAR);
      int len = editor.row[filerow].rsize - editor.coloff;
      if (len < 0) len = 0;
      if (len > editor.screencols) len = editor.screencols;
      char *c = &editor.row[filerow].render[editor.coloff];
      unsigned char *hl = &editor.row[filerow].hl[editor.coloff];
      unsigned char* selected = &(editor.row[filerow].selected[editor.coloff]);
      int current_color = 0;
      int j;
      for (j = 0; j < len; j++) {
        if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abufAppend(ab, ANSI_INVERT);
          abufAppendN(ab, &sym, 1);
          abufAppend(ab, ANSI_CLEAR);
          if (current_color >= 0) {
            char buf[20];
            colorToANSI(editor.cfg->highlight_color[current_color], buf, 0);
            abufAppend(ab, buf);
          }
        } else if (editor.is_selected && selected[j]) {
          if (current_color != -1) {
            current_color = -1;
            abufAppend(ab, ANSI_CLEAR);
            char buf[20];
            colorToANSI(editor.cfg->highlight_color[0], buf, 0);
            abufAppend(ab, ANSI_INVERT);
            abufAppend(ab, buf);
          }
          abufAppendN(ab, &c[j], 1);
        } else {
          int color = hl[j];
          if (color != current_color) {
            current_color = color;
            abufAppend(ab, ANSI_CLEAR);
            char buf[20];
            colorToANSI(editor.cfg->highlight_color[color], buf, 0);
            abufAppend(ab, buf);
          }
          abufAppendN(ab, &c[j], 1);
        }
      }
      abufAppend(ab, ANSI_CLEAR);
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
  row->render = malloc(row->size + tabs * (editor.cfg->tab_size) + 1);

  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % (editor.cfg->tab_size) != 0) row->render[idx++] = ' ';
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

  editor.numrows_digits = 0;
  int num_rows = editor.numrows;
  while (num_rows) {
    num_rows /= 10;
    editor.numrows_digits++;
  }
}

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (editor.cfg->tab_size - 1) - (rx % editor.cfg->tab_size);
    rx++;
  }
  return rx;
}

int editorRowSxToCx(erow* row, int sx) {
  if (sx <= 0) return 0;
  int cx = 0;
  int rx = 0;
  int rx2 = 0;
  while (cx < row->size && rx < sx) {
    rx2 = rx;
    if (row->chars[cx] == '\t') {
      rx += (editor.cfg->tab_size - 1) - (rx % editor.cfg->tab_size);
    }
    rx++;
    cx++;
  }

  if (rx - sx >= sx - rx2) {
    cx--;
  }
  return cx;
}

int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (editor.cfg->tab_size - 1) - (cur_rx % editor.cfg->tab_size);
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}

void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;

  abufAppend(&ab, "\x1b[?25l");

  if (editor.mode == MODE_INSERT) {
    // Insert mode: Change to vertical bar
    abufAppend(&ab, "\x1b[6 q");
  } else {
    // Normal mode: Change to block cursor
    abufAppend(&ab, "\x1b[2 q");
  }

  abufAppend(&ab, "\x1b[H");

  editorScroll();

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);
  editorClearStatusBar();

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", editor.cy - editor.rowoff + 1, (editor.rx - editor.coloff) + 1 + editor.numrows_digits + 1);
  abufAppend(&ab, buf);

  abufAppend(&ab, "\x1b[?25h");

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(editor.statusmsg, sizeof(editor.statusmsg), fmt, ap);
  va_end(ap);
}

void editorClearStatusBar() {
  snprintf(editor.statusmsg, sizeof(editor.statusmsg), "");
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
