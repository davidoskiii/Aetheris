#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "input.h"

#include "../terminal/terminal.h"
#include "../output/output.h"
#include "../utils/utils.h"
#include "../common.h"

void editorMoveCursor(int key) {
  erow *row = (editor.cy >= editor.numrows) ? NULL : &editor.row[editor.cy];

  switch (key) {
    case ARROW_LEFT:
      if (editor.cx != 0) {
        editor.cx--;
      } else if (editor.cy > 0) {
        editor.cy--;
        editor.cx = editor.row[editor.cy].size;
      }
      break;
    case ARROW_RIGHT:
      if (row && editor.cx < row->size) {
        editor.cx++;
      } else if (row && editor.cx == row->size) {
        editor.cy++;
        editor.cx = 0;
      }
      break;
    case ARROW_UP:
      if (editor.cy != 0) {
        editor.cy--;
      }
      break;
    case ARROW_DOWN:
      if (editor.cy < editor.numrows) {
        editor.cy++;
      }
      break;
  }

  row = (editor.cy >= editor.numrows) ? NULL : &editor.row[editor.cy];
  int rowlen = row ? row->size : 0;
  if (editor.cx > rowlen) {
    editor.cx = rowlen;
  }
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  editor.dirty++;
}

void editorInsertChar(int c) {
  if (editor.cy == editor.numrows) {
    editorAppendRow("", 0);
  }
  editorRowInsertChar(&editor.row[editor.cy], editor.cx, c);
  editor.cx++;
}

void editorProcessKeypress() {
  int c = editorReadKey();
  switch (c) {
    case '\r':
      /* TODO */
      break;

    case CTRL_KEY('q'): {
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);

      exit(0);
      break;
    }
    case CTRL_KEY('s'):
      editorSave();
      break;
    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      /* TODO */
      break;
    case CTRL_KEY('l'):
    case '\x1b':
      break;
    case HOME_KEY:
      editor.cx = 0;
      break;
    case END_KEY:
      if (editor.cy < editor.numrows) editor.cx = editor.row[editor.cy].size;
      break;
    case PAGE_UP: {
      if (c == PAGE_UP) {
        editor.cy = editor.rowoff;
      } else if (c == PAGE_DOWN) {
        editor.cy = editor.rowoff + editor.screenrows - 1;
        if (editor.cy > editor.numrows) editor.cy = editor.numrows;
      }
    break;
    }
    case PAGE_DOWN: {
      int times = editor.screenrows;
      while (times--) editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      break;
    }
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;

    default:
      editorInsertChar(c);
      break;
  }
}
