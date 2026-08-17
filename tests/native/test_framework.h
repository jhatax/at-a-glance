#pragma once

#include <stdio.h>

#define TEST_CHECK(condition)                                               \
  do {                                                                      \
    if (!(condition)) {                                                     \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
      return 1;                                                             \
    }                                                                       \
  } while (0)

typedef int (*TestFunction)(void);

typedef struct {
  const char* name;
  TestFunction function;
} TestCase;
