#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#include "../output/output.h"
#include "../common.h"

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void editorOpen(char *filename) {
  free(editor.filename);
  editor.filename = strdup(filename);

  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
    editorAppendRow(line, linelen);
  }
  free(line);
  fclose(fp);
}
