#pragma once

// Debug ingress contract only. This header centralizes debug build gates and
// debug-only runtime test hooks; it is not a catch-all for helper APIs, product
// flags, or runtime behavior.
#ifndef ATAGLANCE_DEBUG
#define ATAGLANCE_DEBUG 0
#endif
