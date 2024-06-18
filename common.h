#ifndef caetheris_common_h
#define caetheris_common_h

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <termios.h>

typedef struct erow {
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;

typedef struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  char *filename;
  erow *row;
  struct termios orig_termios;
} editorConfig;

enum editorKey {
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

extern editorConfig editor;

#define AETHERIS_VERSION "0.0.1"
#define AETHERIS_TAB_STOP 8

#endif
