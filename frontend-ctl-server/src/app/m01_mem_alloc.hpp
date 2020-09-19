
#ifndef M01_MEM_ALLOC_HPP
#define M01_MEM_ALLOC_HPP

#include <stddef.h>


extern int m01_init_mem_pools();

extern void* m01_malloc(size_t size);
extern void* m01_realloc(void* ptr, size_t new_size);
extern void* m01_calloc(size_t count, size_t size);
extern void  m01_free(void* ptr);

extern void m01_print_alloc_free_stats();

#endif  // M01_MEM_ALLOC_HPP
