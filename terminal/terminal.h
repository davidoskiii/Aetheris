#ifndef caetheris_terminal_h
#define caetheris_terminal_h

void disableRawMode();
void enableRawMode();
int editorReadKey();

int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);

#endif
