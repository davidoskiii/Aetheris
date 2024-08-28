#ifndef caetheris_common_h
#define caetheris_common_h

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <termios.h>

typedef struct erow {
  int idx;
  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char *hl;
  int hl_open_comment;
} erow;

typedef struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int raw_screencols;
  int numrows;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  erow *row;
  int dirty;
  int linenum_indent;
  struct editorSyntax *syntax;
  struct termios orig_termios;
} editorConfig;

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

enum editorHighlight {
  HL_NORMAL = 0,
  HL_PAREN,
  HL_FUNCTION,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD,
  HL_IDENTIFIER,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};

struct editorSyntax {
  char *filetype;
  char **filematch;
  char **keywords;
  char *singleline_comment_start;
  char *multiline_comment_start;
  char *multiline_comment_end;
  int flags;
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

extern editorConfig editor;

#define AETHERIS_VERSION "0.0.1"
#define AETHERIS_TAB_STOP 8
#define AETHERIS_QUIT_TIMES 3

#endif
