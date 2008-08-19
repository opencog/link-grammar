#include "fast-sprintf.hpp"

char* fast_sprintf(char* buffer, int num) {
  char* begin = buffer;
  do {
    *buffer++ = '0' + num % 10;
    num /= 10;
  } while (num > 0);
  char* end = buffer - 1;

  for (; begin < end; begin++, end--) {
    char tmp = *begin;
    *begin = *end;
    *end = tmp;
  }

  *buffer = '\0';
  
  return buffer;
}

char* fast_sprintf(char* buffer, const char* str) {
  while(*buffer++ = *str++)
    ;
  return buffer - 1;
}
