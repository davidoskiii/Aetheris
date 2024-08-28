#ifndef caetheris_input_h
#define caetheris_input_h

void editorMoveCursor(int key);
void editorProcessKeypress();
void editorNormalProcessKeypress();

char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorDelRow(int at);

#endif
