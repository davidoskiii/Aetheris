#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#include "../output/output.h"
#include "../input/input.h"
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
    editorInsertRow(editor.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  editor.dirty = 0;
}

void editorSave() {
  if (editor.filename == NULL) {
    editor.filename = editorPrompt("Save as: %s (ESC to cancel)");
    if (editor.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
  }
  int len;
  char *buf = editorRowsToString(&len);
  int fd = open(editor.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        editor.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }
  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < editor.numrows; j++)
    totlen += editor.row[j].size + 1;
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < editor.numrows; j++) {
    memcpy(p, editor.row[j].chars, editor.row[j].size);
    p += editor.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}
