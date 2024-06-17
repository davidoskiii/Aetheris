#include <termios.h>
#include <unistd.h>

#include "terminal/terminal.h"
#include "input/input.h"
#include "common.h"

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
}

int main() {
  enableRawMode();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
