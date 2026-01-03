#include <blog.h>
#include <bgame/allocator.h>
#include <cute.h>
#define BLIB_REALLOC bgame_realloc

#define BLIB_IMPLEMENTATION
#define BENT_LOG BLOG_DEBUG
#define BENT_ASSERT CF_ASSERT
#include <bent.h>
