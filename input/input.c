#include <ctype.h>
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

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  editor.dirty++;
}

void editorRowDelChar(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  editor.dirty++;
}

void editorInsertChar(int c) {
  if (editor.cy == editor.numrows) {
    editorInsertRow(editor.numrows, "", 0);
  }
  editorRowInsertChar(&editor.row[editor.cy], editor.cx, c);
  editor.cx++;
}

void editorDelChar() {
  if (editor.cy == editor.numrows) return;
  if (editor.cx == 0 && editor.cy == 0) return;
  erow *row = &editor.row[editor.cy];
  if (editor.cx > 0) {
    editorRowDelChar(row, editor.cx - 1);
    editor.cx--;
  } else {
    editor.cx = editor.row[editor.cy - 1].size;
    editorRowAppendString(&editor.row[editor.cy - 1], row->chars, row->size);
    editorDelRow(editor.cy);
    editor.cy--;
  }
}

void editorInsertNewline() {
  if (editor.cx == 0) {
    editorInsertRow(editor.cy, "", 0);
  } else {
    erow *row = &editor.row[editor.cy];
    editorInsertRow(editor.cy + 1, &row->chars[editor.cx], row->size - editor.cx);
    row = &editor.row[editor.cy];
    row->size = editor.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  editor.cy++;
  editor.cx = 0;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void editorDelRow(int at) {
  if (at < 0 || at >= editor.numrows) return;
  editorFreeRow(&editor.row[at]);
  memmove(&editor.row[at], &editor.row[at + 1], sizeof(erow) * (editor.numrows - at - 1));
  for (int j = at; j < editor.numrows - 1; j++) editor.row[j].idx--;
  editor.numrows--;
  editor.dirty++;
}

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';
  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();
    int c = editorReadKey();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      editorSetStatusMessage("");
      if (callback) callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback) callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback) callback(buf, c);
  }
}


void editorProcessCommand(const char* command, int quit_times) {
  if (strcmp(command, "w") == 0) {
    editorSave();
  }  else if (strcmp(command, "wq") == 0) {
    editorSave();
    editorQuit();
  }  else if (strcmp(command, "s") == 0) {
    editorFind();
  } else if (strcmp(command, "q") == 0) {
    editorQuitSafe(quit_times);
  } else if (strcmp(command, "q!") == 0) {
    editorQuit();
  } else {
    editorSetStatusMessage("Unknown command '%s'", command);
  }
}

void editorProcessKeypress() {
  static int quit_times = AETHERIS_QUIT_TIMES;
  int c = editorReadKey();
  switch (c) {
    case '\r':
      editorInsertNewline();
      break;

    case CTRL_KEY('t'): {
      char* command = editorPrompt("Type a command: %s (Esc to cancel)", NULL);
      if (command == NULL) {
        editorSetStatusMessage("Command aborted");
        break;
      }

      editorProcessCommand(command, quit_times);
      break;
    }
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
      if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      editorDelChar();
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
  quit_times = AETHERIS_QUIT_TIMES;
}
