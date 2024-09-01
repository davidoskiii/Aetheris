#pragma once
#ifndef caetheris_common_h
#define caetheris_common_h

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <termios.h>

typedef struct ConfigSettings ConfigSettings;

typedef struct erow {
  int idx;
  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char* hl;
  unsigned char* selected;
  int hl_open_comment;
} erow;

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  SHIFT_LEFT,
  SHIFT_RIGHT,
  SHIFT_UP,
  SHIFT_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

typedef struct editorConfig {
  int cx, cy;
  int rx, sx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows_digits;
  int numrows;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  erow *row;
  int dirty;
  int mode;
  int is_selected;
  int select_y, select_x;
  int bracket_autocomplete;
  ConfigSettings *cfg;
  struct editorSyntax *syntax;
  struct termios orig_termios;
} editorConfig;

enum editorMode {
  MODE_INSERT = 0,
  MODE_NORMAL,
  MODE_VISUAL
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
#define AETHERIS_QUIT_TIMES 3

#endif
