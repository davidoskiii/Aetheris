#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "output.h"

#include "../terminal/terminal.h"
#include "../utils/utils.h"
#include "../common.h"

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}
