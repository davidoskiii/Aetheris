#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../output/output.h"
#include "../syntax/syntax.h"
#include "../input/input.h"
#include "../common.h"

enum charType {
  CHAR_ALPHANUM = 1,
  CHAR_SYMBOL,
  CHAR_SPACE,
};


int getCharType(char c) {
  if (isspace(c)) {
    return CHAR_SPACE;
  } else if (is_separator(c)) {
    return CHAR_SYMBOL;
  }
  return CHAR_ALPHANUM;
}

void editorJoinLines() {
  if (editor.cy == editor.numrows - 1)
    return;
  erow *row = &editor.row[editor.cy];
  erow *rowBelow = &editor.row[editor.cy + 1];
  editorRowAppendString(row, " ", 1);
  editorRowAppendString(row, rowBelow->chars, rowBelow->size);
  editorDelRow(editor.cy + 1);
}


/* Vim-like word movement. In vim it seems to move the cursor using any
   transition from word/symbol/whitespace.*/
void editorMoveCursorWordForward() {
  erow *row = &editor.row[editor.cy];
  int cursor = -1;
  int previous_cursor = -1;
  while (editor.cy < editor.numrows) {
    if (editor.cx >= row->size) {  // move down a row
      if (editor.cy == editor.numrows - 1) { // if this is last row keep in bounds
        editor.cx = row->size;
        return;
      }
      editor.cy ++;
      row = &editor.row[editor.cy];
      editor.cx = 0;
    }
    cursor = getCharType(row->chars[editor.cx]);
    if ((cursor != CHAR_SPACE) && (previous_cursor > 0) &&
        ((previous_cursor != cursor) || editor.cx == 0)) {
      // The editor.cx == 0 check above will mean that we're on a new word in a
      // different row.
      break;
    }
    editor.cx ++;
    previous_cursor = cursor;
  }
}

/* Vim-like word movement.
   If the cursor is at the start of a word, go to the start of the preceding
   word. Otherwise, go to the start of _this_ word.
   TODO: ensure this works with moving back in the file etc.
   TODO: these could be rewritten to not mutate cx/cy, but instead to return the
         cursor position for the start of the word.
 */
void editorMoveCursorWordBackward() {
  erow *row = &editor.row[editor.cy];
  erow *previous_row;
  int cursor_type = -1;
  int lookbehind_type = -1;
  int num_type_changes = 0;
  int was_on_first_letter = -1;
  while (editor.cy >=0) {
    row = &editor.row[editor.cy];
    cursor_type = getCharType(row->chars[editor.cx]);
    if (cursor_type != CHAR_SPACE) {

      // lookup the lookbehind
      if (editor.cx >= 1) {
        lookbehind_type = getCharType(row->chars[editor.cx - 1]);
      } else if (editor.cx == 0) {
        lookbehind_type = CHAR_SPACE;
      }

      // set was_on_first_letter
      if (was_on_first_letter == -1 && lookbehind_type >= 0) {
        if (cursor_type == lookbehind_type) {
          was_on_first_letter = 0;
        } else {
          was_on_first_letter = 1;
        }
      }

      // set num changes
      if (cursor_type != lookbehind_type)
        num_type_changes ++;

      // check if should break. If we were on the first letter of an existing
      // word/symbol then we want to move to the start of the NEXT word,
      // otherwise this one.
      if (was_on_first_letter == 0 && num_type_changes == 1)  {
        break;
      } else if (was_on_first_letter == 1 && num_type_changes == 2) {
        break;
      }
    }

    // Move cursor backwards
    // if editor.cx < 0, then set it to end of the above row.
    editor.cx --;
    if (editor.cx < 0) {
      if (editor.cy > 0) {
        editor.cy --;
        previous_row = &editor.row[editor.cy];
        editor.cx = previous_row->size - 1;
      } else {
        editor.cx = 0;
      }
    }
  }
}
