#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "output.h"

#include "../terminal/terminal.h"
#include "../utils/utils.h"
#include "../common.h"

void editorDrawRows() {
  int y;
  for (y = 0; y < 24; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}
