#include <stddef.h>
#include <stdio.h>

#include "test_framework.h"

int test_helper_ranges(void);
int test_helper_math(void);
int test_helper_colors(void);
int test_helper_tuple_parsing(void);
int test_substratum_computations(void);

static const TestCase tests[] = {
    {"helper_ranges", test_helper_ranges},
    {"helper_math", test_helper_math},
    {"helper_colors", test_helper_colors},
    {"helper_tuple_parsing", test_helper_tuple_parsing},
    {"substratum_computations", test_substratum_computations},
};

int main(
    void) {
  int failures = 0;

  for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
    int result = tests[i].function();
    if (result == 0) {
      printf("PASS %s\n", tests[i].name);
    } else {
      printf("FAIL %s\n", tests[i].name);
      failures++;
    }
  }

  printf("%zu tests, %d failures\n", sizeof(tests) / sizeof(tests[0]), failures);
  return failures == 0 ? 0 : 1;
}
