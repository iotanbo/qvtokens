
#include "m01_mem_alloc.hpp"
#include <stdio.h>
#include <stdlib.h>


int m01_init_mem_pools() {

   int result = 0;

   return result; 
}


static size_t mallocs_total = 0;
static size_t callocs_total = 0;
static size_t reallocs_total = 0;
static size_t frees_total = 0;

void* m01_malloc(size_t size) {
    mallocs_total++;
    // return new char(size);
    void* ptr = malloc(size);
    fprintf(stderr, "== m01_malloc(%lu) called: [%lld]\n", size, (long long) ptr);
    return ptr;
}


void* m01_realloc(void* ptr, size_t new_size) {

    reallocs_total++;
    void* new_ptr = realloc(ptr, new_size);

    fprintf(stderr, "== m01_realloc([%lld], %lu) -> [%lld]called\n", 
    (long long) ptr, new_size, (long long) new_ptr);
    return new_ptr;
}


void* m01_calloc(size_t count, size_t size) {
    callocs_total++;
    // size_t total = count * size;
    // void* ptr = m01_malloc(total);
    void* ptr = calloc(count, size);
    fprintf(stderr, "== m01_calloc(%lu, %lu) called: [%lld]\n", count, size, (long long) ptr);
    return ptr;

}


void m01_free(void* ptr) {
    frees_total++;
    fprintf(stderr, "== m01_free(%lld) called\n", (long long) ptr);
    free(ptr);
    // delete[] ptr;
}


void m01_print_alloc_free_stats() {
    size_t allocs_total = mallocs_total + callocs_total + reallocs_total;
    fprintf(stderr, "** TOTAL MEM ALLOCS: %lu (%lu malloc, %lu calloc, %lu realloc)\n", 
        allocs_total, mallocs_total, callocs_total, reallocs_total);
    fprintf(stderr, "** TOTAL MEM FREES: %lu\n", 
        frees_total);
}
