#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <termios.h>
#include <unistd.h>

#include "terminal/terminal.h"
#include "input/input.h"
#include "output/output.h"
#include "utils/utils.h"
#include "common.h"

void initEditor() {
  editor.cx = 0;
  editor.cy = 0;
  editor.rx = 0;
  editor.rowoff = 0;
  editor.coloff = 0;
  editor.numrows = 0;
  editor.row = NULL;
  editor.filename = NULL;
  editor.statusmsg[0] = '\0';
  editor.statusmsg_time = 0;

  if (getWindowSize(&editor.screenrows, &editor.screencols) == -1) die("getWindowSize");
  editor.screenrows -= 2;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();

  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("HELP: Ctrl-Q = quit");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
