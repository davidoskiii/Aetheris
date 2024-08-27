#ifndef caetheris_input_h
#define caetheris_input_h

void editorMoveCursor(int key);
void editorProcessKeypress();

char *editorPrompt(char *prompt);
void editorDelRow(int at);

#endif
