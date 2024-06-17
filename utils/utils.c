#include <termios.h>
#include <unistd.h>

#include "../common.h"

void die(const char *s) {
  perror(s);
  exit(1);
}
