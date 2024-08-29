#ifndef caetheris_config_h
#define caetheris_config_h

#include "../utils/utils.h"
#include "../common.h"

#define ANSI_CLEAR "\x1b[m"
#define ANSI_INVERT "\x1b[7m"
#define ANSI_DEFAULT_FG "\x1b[39m"
#define ANSI_DEFAULT_BG "\x1b[49m"

typedef struct Color {
    int r, g, b;
} Color;

typedef struct EditorConfig {
  int tab_size;
  Color status_color[2];
  Color highlight_color[HL_TYPE_COUNT];
} EditorConfig;

void editorLoadConfig();
void editorSetting();

#endif
