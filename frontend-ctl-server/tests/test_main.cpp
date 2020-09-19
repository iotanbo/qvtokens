
#include <assert.h>
#include "../src/app/app.hpp"
#include <stdio.h>


extern int test_pools();
extern int test_container_of_unique_ptr();

int main(int, char**) {

    fprintf(stderr, "== Starting [qvtoken frontend server ctl] tests ==\n");

    int result = 0;
    result |= test_pools();
    result |= test_container_of_unique_ptr();

    if (0 == result) {
        fprintf(stderr, "[qvtoken frontend server ctl] tests SUCCESSFULLY complete\n");
    } else {
        fprintf(stderr, "[qvtoken frontend server ctl] tests FAILED\n");
    }
    
    return result;
}
