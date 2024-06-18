#ifndef caetheris_output_h
#define caetheris_output_h

#include <unistd.h>

typedef struct abuf abuf;

void editorDrawRows(struct abuf *ab);
void editorAppendRow(char *s, size_t len);
void editorRefreshScreen();
void editorScroll();
void editorSetStatusMessage(const char *fmt, ...);

#endif
