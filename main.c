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
  editor.numrows = 0;

  if (getWindowSize(&editor.screenrows, &editor.screencols) == -1) die("getWindowSize");
}

int main() {
  enableRawMode();
  initEditor();
  editorOpen();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
