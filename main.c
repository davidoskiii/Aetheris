#include <termios.h>
#include <unistd.h>

#include "terminal/terminal.h"
#include "input/input.h"
#include "output/output.h"
#include "common.h"

int main() {
  enableRawMode();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
