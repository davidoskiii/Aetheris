#ifndef caetheris_terminal_h
#define caetheris_terminal_h

#define CTRL_KEY(k) ((k) & 0x1f)

void disableRawMode();
void enableRawMode();
char editorReadKey();

#endif
