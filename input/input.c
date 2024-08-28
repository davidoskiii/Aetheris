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
    erow* row = &(editor.row[editor.cy]);
    if (editor.cx > 0) {
      editorRowDelChar(row, editor.cx - 1);
      editor.cx--;
    } else {
      editor.cx = editor.row[editor.cy - 1].size;
      editorRowAppendString(&(editor.row[editor.cy - 1]), row->chars, row->size);
      editorDelRow(editor.cy);
      editor.cy--;
    }
    editor.sx = editorRowCxToRx(&(editor.row[editor.cy]), editor.cx);
}

void editorInsertNewline() {
  int i = 0;

  if (editor.cx == 0) {
    editorInsertRow(editor.cy, "", 0);
  } else {
    editorInsertRow(editor.cy + 1, "", 0);
    erow* curr_row = &(editor.row[editor.cy]);
    erow* new_row = &(editor.row[editor.cy + 1]);

    while (i < editor.cx && (curr_row->chars[i] == ' ' || curr_row->chars[i] == '\t'))
      i++;
    if (i != 0)
      editorRowAppendString(new_row, curr_row->chars, i);
    if (curr_row->chars[editor.cx - 1] == ':' ||
      (curr_row->chars[editor.cx - 1] == '{' && curr_row->chars[editor.cx] != '}')) {
      editorRowAppendString(new_row, "\t", 1);
      i++;
    }
    editorRowAppendString(new_row, &(curr_row->chars[editor.cx]), curr_row->size - editor.cx);
    curr_row->size = editor.cx;
    curr_row->chars[curr_row->size] = '\0';
    editorUpdateRow(curr_row);
  }
  editor.cy++;
  editor.cx = i;
  editor.sx = editorRowCxToRx(&(editor.row[editor.cy]), i);
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

int editorNormalMovement(int key) {
  switch (key) {
    case 'h': return ARROW_LEFT; break;
    case 'j': return ARROW_DOWN; break;
    case 'k': return ARROW_UP; break;
    case 'l': return ARROW_RIGHT; break;
    case '\r': return ARROW_DOWN; break;
    case ' ': return ARROW_RIGHT; break;
    case BACKSPACE: return ARROW_LEFT; break;
    default: return key;
  }
}

void editorDoInsert(int key) {
  switch (key) {
    case 'i':
      editor.mode = 0;
      break;
    case 'I': 
      editor.cx = 0;
      editor.mode = 0;
      break;
    case 'a': 
      editorMoveCursor(ARROW_RIGHT);
      editor.mode = 0;
      break;
    case 'A': 
      if (editor.cy < editor.numrows) {
        editor.cx = editor.row[editor.cy].size;
      }
      editor.mode = 0;
      break;
    case 'o':
      if (editor.cy < editor.numrows) {
        editor.cx = editor.row[editor.cy].size;
      }
      editorInsertNewline();
      editor.mode = 0;
      break;
    case 'O':
      editor.cx = 0;
      editorInsertNewline();
      editorMoveCursor(ARROW_UP);
      editor.mode = 0;
      break;
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

    case '\x1b':
      editor.mode = 1;
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

int isStopChr(int c, char *s) {
  for (int i = 0; s[i] != '\0'; i += 1) {
    if (c == s[i]) {
      return 1;
    } else if (c == '\0') {
      return 1;
    }
  }
  return 0;
}

void editorSpecialMovement(int key) {
  char *stopChars = " '\"\n()[].#<>";

  switch (key) {
    case 'w':
      while (!isStopChr(editor.row[editor.cy].chars[editor.cx], stopChars)) {
        editorMoveCursor(ARROW_RIGHT);
        while (isStopChr(editor.row[editor.cy].chars[editor.cx + 1], stopChars)) {
          editorMoveCursor(ARROW_RIGHT);
        }
      }
      editorMoveCursor(ARROW_RIGHT);
      break;
    case 'b':
      while (!isStopChr(editor.row[editor.cy].chars[editor.cx], stopChars)) {
        editorMoveCursor(ARROW_LEFT);
        while (isStopChr(editor.row[editor.cy].chars[editor.cx - 1], stopChars)) {
          editorMoveCursor(ARROW_LEFT);
        }
      }
      editorMoveCursor(ARROW_LEFT);
      break;
  
    case '$': {
      if (editor.cy < editor.numrows) editor.cx = editor.row[editor.cy].size;
      editorMoveCursor(ARROW_LEFT);
      break;
    }

    case '^': {
      editor.cx = 0;
      if (editor.row[editor.cy].chars[editor.cx] != ' ') break;
      do {
        editorMoveCursor(ARROW_RIGHT);
      } while (editor.row[editor.cy].chars[editor.cx + 1] == ' ');
      editorMoveCursor(ARROW_RIGHT);
      break;
    }

    case '}':
      editor.cx = 0;
      editorMoveCursor(ARROW_DOWN);
      while (editor.row[editor.cy].size != 0) {
        editorMoveCursor(ARROW_DOWN);
      }
      break;
    case '{':
      editor.cx = 0;
      editorMoveCursor(ARROW_UP);
      while (editor.row[editor.cy].size != 0) {
        editorMoveCursor(ARROW_UP);
      }
      break;
  }
}

void editorNormalProcessKeypress() {
  static int quit_times = AETHERIS_QUIT_TIMES;
  int c = editorReadKey();
  int count = 0;

  while (c >= '0' && c <= '9') {
    count = count * 10 + (c - '0');
    c = editorReadKey();
  }

  if (count == 0) count = 1;

  switch (c) {
    case ':': {
      char* command = editorPrompt("Type a command: %s (Esc to cancel)", NULL);
      if (command == NULL) {
        editorSetStatusMessage("Command aborted");
        break;
      }

      editorProcessCommand(command, quit_times);
      break;
    }


    case 'w':
    case 'b':
    case '$':
    case '^':
    case '}':
    case '{':
      for (int i = 0; i < count; i++) {
        editorSpecialMovement(c);
      }
      break;

    case 'p':
      editorSetStatusMessage("char at %d = %d", editor.rx, editor.row[editor.cy].chars[editor.cx]);
      break;

    case CTRL_KEY('q'):
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

    case CTRL_KEY('s'):
      editorSave();
      break;

    case HOME_KEY:
      editor.cx = 0;
      break;

    case END_KEY:
      if (editor.cy < editor.numrows) {
        editor.cx = editor.row[editor.cy].size;
      }
      break;

    case CTRL_KEY('f'):
      editorFind();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
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
      }
      break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      for (int i = 0; i < count; i++) {
        editorMoveCursor(c);
      }
      break;

    case 'i':
    case 'I':
    case 'a':
    case 'A':
    case 'o':
    case 'O':
      editorDoInsert(c);
      break;

    default:
      editorMoveCursor(editorNormalMovement(c));
      break;
  }

  quit_times = AETHERIS_QUIT_TIMES;
}
