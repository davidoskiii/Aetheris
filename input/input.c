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
      if (editor.cx > AETHERIS_LINE_NUMBER_PREFIX) {
        editor.cx--;
      } else if (editor.cy > 0) {
        editor.cy--;
        editor.cx = editor.row[editor.cy].size + AETHERIS_LINE_NUMBER_PREFIX;
      }
      break;
    case ARROW_RIGHT:
      if (row && editor.cx < row->size + AETHERIS_LINE_NUMBER_PREFIX) {
        editor.cx++;
      } else if (row && editor.cx == row->size + AETHERIS_LINE_NUMBER_PREFIX) {
        editor.cy++;
        editor.cx = AETHERIS_LINE_NUMBER_PREFIX;
      }
      break;
    case ARROW_UP:
      if (editor.cy != 0) {
        editor.cy--;
      }
      break;
    case ARROW_DOWN:
      if (editor.cy < editor.numrows - 1) {
        editor.cy++;
      }
      break;
  }

  row = (editor.cy >= editor.numrows) ? NULL : &editor.row[editor.cy];
  int rowlen = row ? row->size + AETHERIS_LINE_NUMBER_PREFIX : AETHERIS_LINE_NUMBER_PREFIX;
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
  static int quit_times = AETHERIS_QUIT_TIMES;
  int c = editorReadKey();
  switch (c) {
    case '\r':
      /* TODO */
      break;

    case CTRL_KEY('q'): {
      if (editor.dirty && quit_times > 0) {
        editorSetStatusMessage("WARNING!!! File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
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
      editor.cx = AETHERIS_LINE_NUMBER_PREFIX;
      break;
    case END_KEY:
      if (editor.cy < editor.numrows) {
        editor.cx = editor.row[editor.cy].size + AETHERIS_LINE_NUMBER_PREFIX;
      }
      break;
    case PAGE_UP:
    case PAGE_DOWN: {
      if (c == PAGE_UP) {
        editor.cy = editor.rowoff;
      } else if (c == PAGE_DOWN) {
        editor.cy = editor.rowoff + editor.screenrows - 1;
        if (editor.cy > editor.numrows) editor.cy = editor.numrows;
      }

      int times = editor.screenrows;
      while (times--) {
        editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
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
  quit_times = AETHERIS_QUIT_TIMES;
}
