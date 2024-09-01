#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "select.h"

#include "../output/output.h"
#include "../syntax/syntax.h"
#include "../input/input.h"
#include "../config/config.h"
#include "../common.h"

void getSelectStartEnd(int* start_x, int* start_y, int* end_x, int* end_y) {
  if (editor.select_y > editor.cy) {
    *start_x = editor.cx;
    *start_y = editor.cy;
    *end_x = editor.select_x;
    *end_y = editor.select_y;
  }
  else if (editor.select_y < editor.cy) {
    *start_x = editor.select_x;
    *start_y = editor.select_y;
    *end_x = editor.cx;
    *end_y = editor.cy;
  }
  else {
    *start_y = *end_y = editor.cy;
    *start_x = editor.select_x > editor.cx ? editor.cx : editor.select_x;
    *end_x = editor.select_x > editor.cx ? editor.select_x : editor.cx;
  }
}

void editorSelectText() {
  if (!editor.is_selected)
    return;
  for (int i = 0; i < editor.numrows; i++) {
    memset(editor.row[i].selected, 0, editor.row[i].rsize);
  }
  int start_x, start_y, end_x, end_y;
  getSelectStartEnd(&start_x, &start_y, &end_x, &end_y);
  start_x = editorRowCxToRx(&(editor.row[start_y]), start_x);
  end_x = editorRowCxToRx(&(editor.row[end_y]), end_x);

  if (start_y == end_y) {
    memset(&(editor.row[editor.cy].selected[start_x]), 1, end_x - start_x);
    return;
  }

  for (int i = start_y; i <= end_y; i++) {
    if (i == start_y) {
      memset(&(editor.row[i].selected[start_x]), 1, editor.row[i].rsize - start_x);
    }
    else if (i == end_y) {
      memset(editor.row[i].selected, 1, end_x);
    }
    else {
      memset(editor.row[i].selected, 1, editor.row[i].rsize);
    }
  }

}

void editorDeleteSelectText() {
  int start_x, start_y, end_x, end_y;
  getSelectStartEnd(&start_x, &start_y, &end_x, &end_y);
  editor.cx = end_x;
  editor.cy = end_y;
  if (end_y - start_y > 1) {
    for (int i = start_y + 1; i < end_y; i++) {
      editorFreeRow(&(editor.row[i]));
    }
    int removed_rows = end_y - start_y - 1;
    memmove(&(editor.row[start_y + 1]), &(editor.row[end_y]), sizeof(erow) * (editor.numrows - end_y));
    for (int i = start_y + 1; i < editor.numrows - removed_rows; i++) {
        editor.row[i].idx -= removed_rows;
    }
    editor.numrows -= removed_rows;
    editor.cy -= removed_rows;
    editor.dirty++;

    editor.numrows_digits = 0;
    int num_rows = editor.numrows;
    while (num_rows) {
      num_rows /= 10;
      editor.numrows_digits++;
    }
  }
  while (editor.cy != start_y || editor.cx != start_x) {
    editorDelChar();
  }
}

void editorFreeClipboard(EditorClipboard* clipboard) {
  if (!clipboard || !clipboard->size) return;
  for (int i = 0; i < clipboard->size; i++) {
    free(clipboard->chars[i]);
  }
  clipboard->size = 0;
  free(clipboard->chars);
}

void editorCopySelectText() {
  if (!editor.is_selected) return;

  int start_x, start_y, end_x, end_y;
  getSelectStartEnd(&start_x, &start_y, &end_x, &end_y);

  editorFreeClipboard(&editor.clipboard);

  editor.clipboard.size = end_y - start_y + 1;
  editor.clipboard.chars = malloc(sizeof(char*) * editor.clipboard.size);

  // Only one line
  if (start_y == end_y) {
    editor.clipboard.chars[0] = malloc(end_x - start_x + 1);
    memcpy(editor.clipboard.chars[0], &editor.row[start_y].chars[start_x],
       end_x - start_x);
    editor.clipboard.chars[0][end_x - start_x] = '\0';
    return;
  }

  // First line
  size_t size = editor.row[start_y].size - start_x;
  editor.clipboard.chars[0] = malloc(size + 1);
  memcpy(editor.clipboard.chars[0], &editor.row[start_y].chars[start_x], size);
  editor.clipboard.chars[0][size] = '\0';
  // Middle
  for (int i = start_y + 1; i < end_y; i++) {
    size = editor.row[i].size;
    editor.clipboard.chars[i - start_y] = malloc(size + 1);
    memcpy(editor.clipboard.chars[i - start_y], editor.row[i].chars, size);
    editor.clipboard.chars[i - start_y][size] = '\0';
  }
  // Last line
  size = end_x;
  editor.clipboard.chars[end_y - start_y] = malloc(size + 1);
  memcpy(editor.clipboard.chars[end_y - start_y], editor.row[end_y].chars, size);
  editor.clipboard.chars[end_y - start_y][size] = '\0';
}

void editorPasteText() {
  if (!editor.clipboard.size) return;
  int x = editor.cx;
  int y = editor.cy;
  if (editor.clipboard.size == 1) {
    erow* row = &editor.row[y];
    char* paste = editor.clipboard.chars[0];
    size_t paste_len = strlen(paste);

    row->chars = realloc(row->chars, row->size + paste_len + 1);
    memmove(&(row->chars[x + paste_len]), &(row->chars[x]), row->size - x);
    memcpy(&(row->chars[x]), paste, paste_len);
    row->size += paste_len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    editor.dirty++;
    editor.cx += paste_len;
  } else {
    // First line
    int auto_indent = editor.cfg->auto_indent;
    editor.cfg->auto_indent = 0;
    editorInsertNewline();
    editor.cfg->auto_indent = auto_indent;
    editorRowAppendString(&editor.row[y], editor.clipboard.chars[0],
                          strlen(editor.clipboard.chars[0]));
    // Middle
    for (int i = 1; i < editor.clipboard.size - 1; i++) {
      editorInsertRow(y + i, editor.clipboard.chars[i],
                      strlen(editor.clipboard.chars[i]));
    }
    // Last line
    erow* row = &editor.row[y + editor.clipboard.size - 1];
    char* paste = editor.clipboard.chars[editor.clipboard.size - 1];
    size_t paste_len = strlen(paste);

    row->chars = realloc(row->chars, row->size + paste_len + 1);
    memmove(&(row->chars[paste_len]), row->chars, row->size);
    memcpy(row->chars, paste, paste_len);
    row->size += paste_len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);

    editor.cy = y + editor.clipboard.size - 1;
    editor.cx = paste_len;
  }
  editor.sx = editorRowCxToRx(&(editor.row[editor.cy]), editor.cx);
}
