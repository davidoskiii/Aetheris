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
  editor.numrows_digits = 0;
  editor.row = NULL;
  editor.dirty = 0;
  editor.mode = MODE_NORMAL;
  editor.is_selected = 0;
  editor.select_x = 0;
  editor.select_y = 0;
  editor.bracket_autocomplete = 0;
  editor.filename = NULL;
  editor.statusmsg[0] = '\0';
  editor.statusmsg_time = 0;
  editor.syntax = NULL;
  editor.clipboard.size = 0;
  editor.clipboard.chars = NULL;

  if (getWindowSize(&editor.screenrows, &editor.screencols) == -1) die("getWindowSize");
  editor.screenrows -= 3;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  enableSwap();

  editorLoadConfig();
  initEditor();

  if (argc >= 2) {
    editorOpen(argv[1]);
  } else {
    editorInsertRow(editor.cy, "", 0);
    editor.dirty = 0;
  }

  editor.screencols -= editor.numrows_digits + 1;

  editorSetStatusMessage("HELP: Ctrl-Q = quit | Ctrl-S = save | Ctrl-F  = find");

  while (1) {
    editorRefreshScreen();
    switch (editor.mode) {
      case MODE_NORMAL: 
        editorNormalProcessKeypress();
        break;
      case MODE_INSERT: 
        editorProcessKeypress();
        break;
      case MODE_VISUAL: 
        editorVisualProcessKeypress();
        break;
      case MODE_VISUAL_LINE: 
        editorVisualLineProcessKeypress();
        break;
    }
  }

  return 0;
}
