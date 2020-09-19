
#include "app.hpp"


int M01FrontendConnContextPool::create(size_t size) {
    pool = new M01FrontendConnContext* [size];
    try {
        for (size_t i=0; i<size; ++i) {
            M01FrontendConnContext* c = new M01FrontendConnContext();
            pool[pool_count++] = c;
        }

    } catch(std::exception& e) {
        return pool_count;
    }
        
    return 0;
}


M01FrontendConnContextPool::~M01FrontendConnContextPool() {

    for(int i=0; i<pool_count; ++i) {
        // Delegate to M01FrontendConnContext freeing its internal structs.
        delete pool[i];
    }
    delete[] pool;
    pool = nullptr;
    // fprintf(stderr, " * M01FrontendConnContextPool DESTRUCTOR complete\n");
    pool_count = 0;
}

M01FrontendConnContext* M01FrontendConnContextPool::get_one() {
    if (!pool_count) { return nullptr; }
    M01FrontendConnContext* c = pool[--pool_count];
    return c;
}

void M01FrontendConnContextPool::recycle(M01FrontendConnContext* conn_context) {

    // TODO: put [authorized_buf_mem] into the pool
    pool[pool_count++] = conn_context;
}
