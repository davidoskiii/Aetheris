#ifndef caetheris_terminal_h
#define caetheris_terminal_h

void disableRawMode();
void enableRawMode();
char editorReadKey();

int getWindowSize(int *rows, int *cols);

#endif
