#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <termios.h>
#include <unistd.h>

#include "terminal/terminal.h"
#include "config/config.h"
#include "input/input.h"
#include "output/output.h"
#include "utils/utils.h"
#include "common.h"

void initEditor() {
  editor.cx = 0;
  editor.cy = 0;
  editor.rx = 0;
  editor.sx = 0;
  editor.rowoff = 0;
  editor.coloff = 0;
  editor.numrows = 0;
  editor.row = NULL;
  editor.dirty = 0;
  editor.mode = MODE_NORMAL;
  editor.is_selected = 0;
  editor.select_x = 0;
  editor.select_y = 0;
  editor.linenum_indent = 6;
  editor.filename = NULL;
  editor.statusmsg[0] = '\0';
  editor.statusmsg_time = 0;
  editor.syntax = NULL;

  if (getWindowSize(&editor.screenrows, &editor.raw_screencols) == -1) die("getWindowSize");
  editor.screenrows -= 2;
  editor.screencols -= editor.raw_screencols;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  editorLoadConfig();
  initEditor();

  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("HELP: Ctrl-Q = quit | Ctrl-S = save | Ctrl-F  = find");

  while (1) {
    editorRefreshScreen();
    if (editor.mode == MODE_NORMAL) {
      editorNormalProcessKeypress();
    } else if (editor.mode == MODE_VISUAL) {
      editorVisualProcessKeypress();
    } else {
      editorProcessKeypress();
    }
  }

  return 0;
}
