#pragma once
#ifndef caetheris_select_h
#define caetheris_select_h

#include <stddef.h>

typedef struct EditorClipboard {
  size_t size;
  char** chars;
} EditorClipboard;

void getSelectStartEnd(int* start_x, int* start_y, int* end_x, int* end_y);
void getSelectStartEnd(int* start_x, int* start_y, int* end_x, int* end_y);
void editorSelectText();
void editorSelectText();
void editorDeleteSelectText();
void editorDeleteSelectText();
void editorCopySelectText();
void editorPasteText();
void editorFreeClipboard(EditorClipboard* clipboard);

#endif
