#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#include "../common.h"

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void editorOpen() {
  char *line = "Hello, world!";
  ssize_t linelen = 13;

  editor.row.size = linelen;
  editor.row.chars = malloc(linelen + 1);
  memcpy(editor.row.chars, line, linelen);
  editor.row.chars[linelen] = '\0';
  editor.numrows = 1;
}
