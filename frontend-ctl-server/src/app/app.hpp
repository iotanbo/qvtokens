#ifndef M01APP_HPP
#define M01APP_HPP

extern "C" {
#include "../../../../libuv/include/uv.h"
}

#include <iostream>
#include <sstream>
#include <memory>
#include "m01_enums_and_defs.hpp"


class M01FrontendConnContext {

public:

    // Define user object type so that uv callbacks know 
    // what type of object they got as a [data] field.
    // ! This must be the first field in the struct!
    M01ObjType user_obj_type = M01OBJ_FRONTEND_CONN_CONTEXT;

    // Buffer used during authorization
    char initial_buf_mem[M01_FRONTEND_CONN_INITIAL_BUF_SIZE];

    // size_t initial_buf_mem_size;
    uv_buf_t rx_buf;

    uv_buf_t tx_buf;
    uv_write_t tx_response;
    size_t tx_response_size = sizeof(uv_write_t);

    uv_buf_t upstream_rx_buf;
    uv_buf_t upstream_tx_buf;

    // Memory allocated after connection is authorized
    std::unique_ptr<char*> authorized_buf_mem = nullptr;
    size_t authorized_buf_mem_size = 0;

    // Current frontend connection
    uv_tcp_t connection;

public: 
    // Constructors
    M01FrontendConnContext() {

        rx_buf.base = initial_buf_mem;
        tx_buf.base = initial_buf_mem + (M01_FRONTEND_CONN_INITIAL_BUF_SIZE/2);

        // Store pointer to this object in the connection struct
        // so that any callback can access it
        connection.data = this;
    }

    ~M01FrontendConnContext() {
        // Both tx and rx buffers share one memory block at start
        // fprintf(stderr, " * ~M01FrontendConnContext DESTRUCTOR called\n");
    }
};


class M01FrontendConnContextPool {
public:

    // TODO: try to use list of unique_ptr!
    // std::forward_list<M01FrontendConnContext*> pool;

    // Create a pool of frontend connections of specified size.
    int create(size_t size);

    M01FrontendConnContext* get_one();

    void recycle(M01FrontendConnContext* conn_context);

    int count() { return pool_count; }

    // Current number of elements in the pool
    // int count = 0;

public:
    // Constructors
    ~M01FrontendConnContextPool();

private:
    int pool_count = 0;
    M01FrontendConnContext** pool = nullptr;
};


class M01App {

public:
    size_t conn_count = 0;
    M01FrontendConnContextPool frontend_conn_pool;

public:

    M01App() { 
            std::cout <<"* M01App constructor"; 
            frontend_conn_pool.create(12);
        }

    // uv_buf_t sayHello() {
    //     std::stringstream ss;
    //     ss <<"* M01App says Hello: conn[" << conn_count << "]\n";
    //     uv_buf_t result;
    //     result.base = ss.str().c_str();
    //     result.len = ss.str().size();
    //     return result;
    // }

};

#endif
