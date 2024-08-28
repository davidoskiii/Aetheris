#ifndef caetheris_utils_h
#define caetheris_utils_h

#define CTRL_KEY(k) ((k) & 0x1f)

void die(const char *s);
int is_integer(const char *str);

void editorQuit();
void editorQuitSafe(int quit_times);
void editorOpen(char *filename);
void editorSave();
void editorFind();
void editorGotoLine(char* query);

void getSelectStartEnd(int* start_x, int* start_y, int* end_x, int* end_y);
void editorDeleteSelectText();
void editorSelectText();

char *editorRowsToString(int *buflen);

#endif
