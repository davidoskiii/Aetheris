#ifndef caetheris_output_h
#define caetheris_output_h

#include <unistd.h>

#include "../common.h"

typedef struct abuf abuf;

void editorDrawRows(struct abuf *ab);
void editorInsertRow(int at, char *s, size_t len);
void editorUpdateRow(erow *row);

int editorRowRxToCx(erow *row, int rx);
int editorRowCxToRx(erow *row, int cx);

void editorRefreshScreen();
void editorScroll();
void editorSetStatusMessage(const char *fmt, ...);

#endif
