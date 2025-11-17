#pragma once

#include <std/string/strslice.h>
#include <stdint.h>

typedef struct utf8 {
  uint32_t codepoint;
  uint8_t width;
} utf8_t;

typedef struct chars {
  strslice_t slice;
  size_t offset;
};
