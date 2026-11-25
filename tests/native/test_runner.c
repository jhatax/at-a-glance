#include "test_runner.h"

#include <pebble.h>
#include <stdarg.h>

static char context[256];
static char messages[4096];
static size_t used;
static const char* skip_reason;
static bool truncated;

void test_skip(
    const char* reason) {
  skip_reason = reason;
  cl_skip();
}

void test_context(
    const char* format,
    ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(context, sizeof(context), format, args);
  va_end(args);
}

static void append(
    const char* format,
    va_list args) {
  if (used >= sizeof(messages) - 1) {
    truncated = true;
    return;
  }
  int count = vsnprintf(messages + used, sizeof(messages) - used, format, args);
  if (count < 0) return;
  size_t available = sizeof(messages) - used;
  if ((size_t)count >= available) truncated = true;
  used += (size_t)count < available ? (size_t)count : available - 1;
  if (used < sizeof(messages) - 1) messages[used++] = '\n';
  messages[used] = '\0';
}

void test_note(
    const char* format,
    ...) {
  va_list args;
  va_start(args, format);
  append(format, args);
  va_end(args);
}

void test_app_log(
    int level,
    const char* format,
    ...) {
  (void)level;
  va_list args;
  va_start(args, format);
  append(format, args);
  va_end(args);
}

static void trace(
    cl_trace_event event,
    const char* suite,
    const char* name,
    void* payload) {
  (void)payload;
  if (event == CL_TRACE__TEST__BEGIN) {
    context[0] = messages[0] = '\0';
    skip_reason = NULL;
    truncated = false;
    used = 0;
  } else if (event == CL_TRACE__TEST__LONGJMP) {
    if (skip_reason) {
      fprintf(stderr, "Skipped: %s\n", skip_reason);
      return;
    }
    if (context[0]) fprintf(stderr, "Context: %s\n", context);
    if (used) fprintf(stderr, "Captured calls and application logs:\n%s", messages);
    if (truncated) fprintf(stderr, "[captured log truncated at %zu bytes]\n", sizeof(messages) - 1);
    fprintf(stderr, "Rerun: npm run test:native -- --suite ");
    // Clar renders underscores in suite names as double colons. Restore the filename.
    for (const char* cursor = suite; *cursor; ++cursor) {
      if (cursor[0] == ':' && cursor[1] == ':') {
        fputc('_', stderr);
        ++cursor;
      } else {
        fputc(*cursor, stderr);
      }
    }
    fprintf(
        stderr,
        " --case %s --platform %s --health %s%s\n",
        name,
        TEST_PLATFORM,
#ifdef PBL_HEALTH
        "health",
#else
        "no-health",
#endif
#ifdef TEST_SANITIZE
        " --sanitize"
#else
        ""
#endif
    );
  }
}

// Every native test process starts here. Clar owns selection and case lifecycle.
int main(
    int argc,
    char** argv) {
  cl_trace_register(trace, NULL);
  return clar_test(argc, argv);
}
