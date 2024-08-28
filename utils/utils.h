#ifndef caetheris_utils_h
#define caetheris_utils_h

#define CTRL_KEY(k) ((k) & 0x1f)

void die(const char *s);

void editorQuit();
void editorQuitSafe(int quit_times);
void editorOpen(char *filename);
void editorSave();
void editorFind();

char *editorRowsToString(int *buflen);

#endif
