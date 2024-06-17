#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "input.h"

#include "../terminal/terminal.h"
#include "../utils/utils.h"
#include "../common.h"

void editorProcessKeypress() {
  char c = editorReadKey();
  switch (c) {
    case CTRL_KEY('q'): {
      write(STDOUT_FILENO, "\x1b[2J", 4);
      exit(0);
      break;
    }
  }
}
