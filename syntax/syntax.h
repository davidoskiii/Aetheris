#pragma once
#ifndef caetheris_syntax_h
#define caetheris_syntax_h

#include "../common.h"

extern char* C_HL_extensions[];
extern char* C_HL_keywords[];

extern struct editorSyntax HLDB[];

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

void editorUpdateSyntax(erow *row);
void editorSelectSyntaxHighlight();

#endif
