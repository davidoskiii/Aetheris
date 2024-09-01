#pragma once
#ifndef caetheris_config_h
#define caetheris_config_h

#include "../utils/utils.h"
#include "../common.h"

#define ANSI_CLEAR "\x1b[m"
#define ANSI_INVERT "\x1b[7m"
#define ANSI_DEFAULT_FG "\x1b[39m"
#define ANSI_DEFAULT_BG "\x1b[49m"

enum editorHighlight {
  HL_NORMAL = 0,
  HL_PAREN,
  HL_FUNCTION,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD,
  HL_MACRO,
  HL_IDENTIFIER,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH,
  HL_TYPE_COUNT
};

typedef struct ConfigSettings {
  int tab_size;
  Color status_color[2];
  Color highlight_color[HL_TYPE_COUNT];
} ConfigSettings;

void editorLoadConfig();
void editorSetting();

#endif
