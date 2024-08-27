#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "syntax.h"

#include "../utils/utils.h"
#include "../output/output.h"

void editorUpdateSyntax(erow *row) {
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);
  int i;
  for (i = 0; i < row->rsize; i++) {
    if (isdigit(row->render[i])) {
      row->hl[i] = HL_NUMBER;
    }
  }
}

int editorSyntaxToColor(int hl) {
  switch (hl) {
    case HL_NUMBER: return 33;
    default: return 37;
  }
}
