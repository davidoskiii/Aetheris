#ifndef caetheris_common_h
#define caetheris_common_h

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <termios.h>

typedef struct editorConfig {
  struct termios orig_termios;
} editorConfig;

extern editorConfig editor;

#endif
