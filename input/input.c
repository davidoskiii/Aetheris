#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "input.h"

#include "../terminal/terminal.h"
#include "../utils/utils.h"
#include "../common.h"

void editorMoveCursor(int key) {
  switch (key) {
    case ARROW_LEFT:
      if (editor.cx != 0) {
        editor.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (editor.cx != editor.screencols - 1) {
        editor.cx++;
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
}

void editorProcessKeypress() {
  int c = editorReadKey();
  switch (c) {
    case CTRL_KEY('q'): {
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);

      exit(0);
      break;
    }
    case HOME_KEY:
      editor.cx = 0;
      break;
    case END_KEY:
      editor.cx = editor.screencols - 1;
      break;
    case PAGE_UP:
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
  }
}
