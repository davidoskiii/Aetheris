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

void editorOpen(char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  linelen = getline(&line, &linecap, fp);
  if (linelen != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
      linelen--;
    }
    editor.row.size = linelen;
    editor.row.chars = malloc(linelen + 1);
    memcpy(editor.row.chars, line, linelen);
    editor.row.chars[linelen] = '\0';
    editor.numrows = 1;
  }
  free(line);
  fclose(fp);
}

