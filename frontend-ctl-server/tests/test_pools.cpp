
#include "../src/app/app.hpp"
#include <assert.h>


int _test_frontend_conn_context_pool_creation() {
    int result = 0;

    M01FrontendConnContextPool pool;
    assert(pool.count() == 0);
    const size_t TEST_POOL_SIZE = 10;

    pool.create(TEST_POOL_SIZE);
    assert(pool.count() == TEST_POOL_SIZE);

    return result; 
}


int test_pools() {
    int result = 0;

    result |= _test_frontend_conn_context_pool_creation();

    return result;
}