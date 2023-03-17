/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "config.h"
#if HAVE_LIBGC
#include <gc/gc.h>
#include <gc/gc_cpp.h>
#include <gc/gc_mark.h>
#endif /* HAVE_LIBGC */
#include <sys/mman.h>

#include <cstddef>
#include <cstring>
#include <new>

#include "backtrace.h"
#include "cstring.h"
#include "gc.h"
#include "log.h"
#include "n4.h"

// One can disable the GC, e.g., to run under Valgrind, by editing config.h
#if HAVE_LIBGC
// emergency pool to allow a few extra allocations after a bad_alloc is thrown so we
// can generate reasonable errors, a stack trace, etc
static char emergency_pool[16 * 1024];
static char *emergency_ptr;

void *operator new(std::size_t size) {
    /* DANGER -- on OSX, can't safely call the garbage collector allocation
     * routines from a static global constructor without manually initializing
     * it first.  Since we have global constructors that want to allocate
     * memory, we need to force initialization */
    auto *rv = gc::operator new(size);
    if ((rv == nullptr) && (emergency_ptr != nullptr) &&
        emergency_ptr + size < emergency_pool + sizeof(emergency_pool)) {
        rv = emergency_ptr;
        size += -size & 0xf;  // align to 16 bytes
        emergency_ptr += size;
    }
    if (rv == nullptr) {
        if (emergency_ptr == nullptr) emergency_ptr = emergency_pool;
        throw backtrace_exception<std::bad_alloc>();
    }
    return rv;
}

void operator delete(void *p) noexcept {
    if (p >= emergency_pool && p < emergency_pool + sizeof(emergency_pool)) {
        return;
    }
    gc::operator delete(p);
}

void operator delete(void *p, std::size_t /*size*/) noexcept {
    if (p >= emergency_pool && p < emergency_pool + sizeof(emergency_pool)) {
        return;
    }
    gc::operator delete(p);
}

void *operator new[](std::size_t size) { return gc::operator new(size); }
void operator delete[](void *p) noexcept { gc::operator delete(p); }
void operator delete[](void *p, std::size_t /*size*/) noexcept { ::operator delete(p); }

#if HAVE_GC_PRINT_STATS
/* GC_print_stats is not exported as an API symbol and cannot be used on some systems */
extern "C" int GC_print_stats;
#endif /* HAVE_GC_PRINT_STATS */

static int gc_logging_level;

static void gc_callback() {
    if (gc_logging_level >= 1) {
        std::clog << "****** GC called ****** (heap size " << n4(GC_get_heap_size()) << ")";
        size_t count = 0;
        size_t size = cstring::cache_size(count);
        std::clog << " cstring cache size " << n4(size) << " (count " << n4(count) << ")"
                  << std::endl;
    }
}

void silent(char *, GC_word) {}

void reset_gc_logging() {
    gc_logging_level = Log::Detail::fileLogLevel(__FILE__);
#if HAVE_GC_PRINT_STATS
    GC_print_stats = gc_logging_level >= 2;  // unfortunately goes directly to stderr!
#endif                                       /* HAVE_GC_PRINT_STATS */
}

#endif /* HAVE_LIBGC */

void setup_gc_logging() {
#if HAVE_LIBGC
    GC_set_start_callback(gc_callback);
    reset_gc_logging();
    Log::Detail::addInvalidateCallback(reset_gc_logging);
    GC_set_warn_proc(&silent);
#endif /* HAVE_LIBGC */
}

size_t gc_mem_inuse(size_t *max) {
#if HAVE_LIBGC
    GC_word heapsize = 0;
    GC_word heapfree = 0;
    GC_gcollect();
    GC_get_heap_usage_safe(&heapsize, &heapfree, nullptr, nullptr, nullptr);
    if (max != nullptr) {
        *max = heapsize;
    }
    return heapsize - heapfree;
#else
    if (max) {
        *max = 0;
    }
    return 0;
#endif
}
