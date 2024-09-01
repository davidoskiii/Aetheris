#pragma once
#ifndef caetheris_input_h
#define caetheris_input_h

#include "../common.h"

void editorMoveCursor(int key);
void editorProcessKeypress();
void editorNormalProcessKeypress();
void editorVisualProcessKeypress();
void editorVisualLineProcessKeypress();

void editorInsertNewline();
void editorRowAppendString(erow *row, char *s, size_t len);
char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorDelRow(int at);
void editorDelChar();
void editorFreeRow(erow *row);

#endif
