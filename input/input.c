#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "input.h"

#include "../terminal/terminal.h"
#include "../utils/utils.h"
#include "../common.h"

void editorMoveCursor(char key) {
  switch (key) {
    case 'a':
      editor.cx--;
      break;
    case 'd':
      editor.cx++;
      break;
    case 'w':
      editor.cy--;
      break;
    case 's':
      editor.cy++;
      break;
  }
}

void editorProcessKeypress() {
  char c = editorReadKey();
  switch (c) {
    case CTRL_KEY('q'): {
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);

      exit(0);
      break;
    }
    case 'w':
    case 's':
    case 'a':
    case 'd':
      editorMoveCursor(c);
      break;
  }
}
