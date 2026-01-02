#include <blog.h>
#include <bgame/allocator.h>
#define BLIB_REALLOC bgame_realloc

#define BLIB_IMPLEMENTATION
#define BENT_LOG BLOG_DEBUG
#include <bent.h>
