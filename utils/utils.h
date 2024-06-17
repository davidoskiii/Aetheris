#ifndef caetheris_utils_h
#define caetheris_utils_h

#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#define CTRL_KEY(k) ((k) & 0x1f)

void die(const char *s);

#endif
