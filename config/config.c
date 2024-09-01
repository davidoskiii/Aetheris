#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "config.h"
#include "../input/input.h"
#include "../utils/utils.h"
#include "../output/output.h"

static ConfigSettings cfg = {.tab_size = 4,
                           .status_color = {{229, 229, 229}, {96, 59, 116}},
                           .highlight_color = {{212, 212, 212},
                                               {106, 153, 85},
                                               {106, 153, 85},
                                               {197, 134, 192},
                                               {86, 156, 214},
                                               {78, 201, 176},
                                               {206, 145, 120},
                                               {181, 206, 168},
                                               {201, 79, 103}}};

static int parseLine(char* line, int verbose) {
  char* token = strtok(line, " ");
  int argc = 0;
  char* argv[4];
  for (int i = 0; token && i < 4; i++, argc++) {
    argv[i] = token;
    token = strtok(NULL, " ");
  }

  if (argc < 1)
    return 0;

  if (strcmp(argv[0], "tabsize") == 0) {
    if (argc != 2) {
      if (verbose)
        editorSetStatusMessage("Usage: tabsize [size]");
      return 0;
    }
    int size = atoi(argv[1]);
    if (size < 1)
      return 0;
    editor.cfg->tab_size = size;
  } else if (strcmp(argv[0], "color") == 0) {
    if (argc != 3) {
      if (verbose)
        editorSetStatusMessage("Usage: color [target] [color]");
      return 0;
    }
    Color color = strToColor(argv[2]);
    if (strcmp(argv[1], "status.fg") == 0) {
        editor.cfg->status_color[0] = color;
    } else if (strcmp(argv[1], "status.bg") == 0) {
        editor.cfg->status_color[1] = color;
    } else if (strcmp(argv[1], "hl.normal") == 0) {
        editor.cfg->highlight_color[HL_NORMAL] = color;
    } else if (strcmp(argv[1], "hl.comment") == 0) {
        editor.cfg->highlight_color[HL_COMMENT] = color;
        editor.cfg->highlight_color[HL_MLCOMMENT] = color;
    } else if (strcmp(argv[1], "hl.keyword") == 0) {
        editor.cfg->highlight_color[HL_KEYWORD] = color;
    } else if (strcmp(argv[1], "hl.macro") == 0) {
        editor.cfg->highlight_color[HL_MACRO] = color;
    } else if (strcmp(argv[1], "hl.function") == 0) {
        editor.cfg->highlight_color[HL_FUNCTION] = color;
    } else if (strcmp(argv[1], "hl.string") == 0) {
        editor.cfg->highlight_color[HL_STRING] = color;
    } else if (strcmp(argv[1], "hl.number") == 0) {
        editor.cfg->highlight_color[HL_NUMBER] = color;
    } else if (strcmp(argv[1], "hl.match") == 0) {
        editor.cfg->highlight_color[HL_MATCH] = color;
    } else {
      if (verbose)
        editorSetStatusMessage("Unknown target %s.", argv[1]);
      return 0;
    }
  } else {
    if (verbose)
      editorSetStatusMessage("Unknown config %s.", argv[0]);
    return 0;
  }
  return 1;
}

void editorLoadConfig() {
  char path[255] = "";
  strncpy(path, getenv("HOME"), sizeof(path));
  strncat(path, "/.aetherisrc", sizeof(path) - 1);
  editor.cfg = &cfg;
  FILE* fp = fopen(path, "r");
  if (!fp)
    return;

  char buf[128];
  while (fgets(buf, sizeof(buf), fp)) {
    buf[strcspn(buf, "\r\n")] = '\0';
    parseLine(buf, 0);
  }

  fclose(fp);
}

void editorSetting() {
  char* query = editorPrompt("Config: %s", NULL);
  if (query == NULL)
    return;
  if (parseLine(query, 1)) {
    for (int i = 0; i < editor.numrows; i++) {
      editorUpdateRow(&editor.row[i]);
    }
  }
  if (query) {
    free(query);
  }
}
