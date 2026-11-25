#pragma once
#include "clar.h"

void test_context(
    const char* format,
    ...);
void test_note(
    const char* format,
    ...);
void test_skip(const char* reason);
