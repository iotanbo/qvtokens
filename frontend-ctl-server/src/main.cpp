/**
 * Qvtokens frontend control server.
 * 
 */


extern "C" {
#include "../../../libuv/include/uv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For some strange reason SO_REUSEPORT is not defined in <sys/socket.h>
//#include <sys/socket.h>
#include <asm-generic/socket.h>
}

#include <iostream>
#include <memory>
#include "app/app.hpp"
#include "app/m01_enums_and_defs.hpp"
#include "app/m01_mem_alloc.hpp"


#define DEFAULT_PORT 55777

// https://stackoverflow.com/questions/36594400/what-is-backlog-in-tcp-connections
#define TCP_BACKLOG_SIZE 5  // 128 is max on Ubuntu



void allocate_rx_buf(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    
    M01FrontendConnContext* ctx = (M01FrontendConnContext*) handle->data;
    if (!ctx) {
        buf->base = nullptr;
        buf->len = 0;
        return;
    }
    buf->base = ctx->rx_buf.base;
    buf->len = MIN(64, suggested_size);

    // fprintf(stderr, " --> allocate_rx_buf() suggested: %ld, allocated: %ld.\n", 
    //         suggested_size, buf->len);
}


void on_conn_close_complete(uv_handle_t *conn) {

    M01App* app = (M01App*) uv_default_loop()->data;
    M01FrontendConnContext* ctx = (M01FrontendConnContext*) conn->data;

    app->frontend_conn_pool.recycle(ctx);
    app->conn_count--;
    fprintf(stderr, "* [on_conn_close_complete] conn [%lld], %ld active connections left\n", 
        (long long) conn, app->conn_count);
}


int finalize_frontend_conn(uv_tcp_t* conn, bool reset=true) {

    if (uv_is_closing( (const uv_handle_t*) conn)) {
        fprintf(stderr, "* ERROR: tried to finalize conn [%lld] that is already CLOSING!\n", 
            (long long) conn);
        return -1;
    }

    if (reset) {
        fprintf(stderr, " --> finalizing WITH RESET client conn [%lld].\n",
        (long long) conn);
        uv_tcp_close_reset((uv_tcp_t*) conn, on_conn_close_complete);
    } else {
        fprintf(stderr, " --> finalizing client conn [%lld].\n",
        (long long) conn);
        uv_close((uv_handle_t*) conn, on_conn_close_complete);
    }
    
    return 0;
}


void try_close_conn(uv_handle_t* handle)
{
    if (handle != NULL) {
        fprintf(stderr, "  * Trying to close handle: %lld of type: [%d]\n", 
            (long long) handle, handle->type);

        // If user data set
        if (handle->data) {
            // Check user object type
            M01ObjType *user_obj_type = (M01ObjType*) handle->data;
            if (*user_obj_type == M01OBJ_FRONTEND_CONN_CONTEXT) {
                M01FrontendConnContext* ctx = (M01FrontendConnContext*) handle->data;
                fprintf(stderr, 
                    "  * Handle of type [M01OBJ_FRONTEND_CONN_CONTEXT], finalizing conn.\n"); 
                finalize_frontend_conn(&ctx->connection);
            } else {
                fprintf(stderr, "  * ERROR: Unknown M01ObjType: %d\n", 
                    (int) *user_obj_type); 
            }
        } else {
           fprintf(stderr, "  * Handle: %lld data is NULL\n", 
            (long long) handle); 
        }
    }
}


void on_close_active_connection(uv_handle_t* handle, void* arg)
{
    try_close_conn(handle);
}


void on_close_active_handler(uv_handle_t* handle, void* arg)
{
    // Close the handler
    uv_close(handle, try_close_conn);
}


static inline int _check_for_cmd(const char* cmd, uv_buf_t* buf) {
    size_t len = strlen(cmd);
    // fprintf(stderr, "  * Received data size: %ld\n", buf->len);
    if (buf->len < len) { return 0; }
    return 0 == memcmp(buf->base, cmd, len);
}


void start_graceful_shutdown() {
    fprintf(stderr, "==> Shutting down the server...\n");
    // Try close all active connections
    uv_walk(uv_default_loop(), on_close_active_connection, NULL);
    fprintf(stderr, "==> Before stopping loop ====\n");
    uv_stop(uv_default_loop());
    // The rest will be done in main()
}


void echo_read(uv_stream_t *conn, ssize_t nread, const uv_buf_t *buf) {

    M01App* app = (M01App*) uv_default_loop()->data;
    M01FrontendConnContext* ctx = (M01FrontendConnContext*) conn->data;

    if (nread > 0) {
        uv_buf_t& rx_buf = ctx->rx_buf = uv_buf_init(buf->base, nread);

        if (_check_for_cmd("quit", &rx_buf)) {
            fprintf(stderr, "==> Got quit request\n");
            // finalize_frontend_conn((uv_tcp_t*) conn);
            start_graceful_shutdown();
            return;
        } else if (_check_for_cmd("close", &rx_buf)) {
            fprintf(stderr, "==> Got close request, closing connection.\n");
            // Recycle connection context
            finalize_frontend_conn((uv_tcp_t*) conn);
            return;
        }
        
        // Get tx buf from connection context
        uv_buf_t& tx_buf = ctx->tx_buf;
        sprintf(tx_buf.base, " --> conn [%ld] received: ", app->conn_count);
        tx_buf.len = strlen(tx_buf.base);
        memcpy(tx_buf.base + tx_buf.len, rx_buf.base, rx_buf.len);
        tx_buf.len += rx_buf.len;
        uv_write((uv_write_t*) &ctx->tx_response, conn, &tx_buf, 1, NULL);  // &req->buf  // echo_write
        return;
    }
    else if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
            finalize_frontend_conn((uv_tcp_t*) conn);
            return;
        } else {
            fprintf(stderr, "* Conn CLOSED by remote peer\n");
            finalize_frontend_conn((uv_tcp_t*) conn, false);
        }
        
    } else {  // nread == 0
        fprintf(stderr, "* Conn nread is 0 (EAGAIN or EWOULDBLOCK)\n");
    }
}


static void on_do_free_timer(uv_handle_t* handle) {
  free(handle);
  fprintf(stderr, "* Timer disposed: [%lld]\n", (long long) handle);
}


static void do_delayed_accept(uv_timer_t* timer_handle) {
    uv_stream_t* server;

    server = (uv_stream_t*) timer_handle->data;

    // Dispose the timer
    uv_close((uv_handle_t*)timer_handle, on_do_free_timer);

    M01App* app = (M01App*) uv_default_loop()->data;

    // Assign frontend conn_context
    M01FrontendConnContext* ctx = app->frontend_conn_pool.get_one();
    if (!ctx) {
        fprintf(stderr, "* Error (DELAYED ACCEPT): no CTX objects left in pool, returning.\n");
        return;
    }

    uv_tcp_t *conn = &ctx->connection;
 
    fprintf(stderr, "* (DELAYED ACCEPT) Opening new conn: [%lld]\n", 
            (long long) conn);

    app->conn_count++;

    uv_tcp_init(uv_default_loop(), conn);
    int result = uv_accept(server, (uv_stream_t*) conn);
    if (result == 0) {
        // Reset the connection if even after the delay 
        // there is not enough empty connections
        if (app->frontend_conn_pool.count() < TCP_BACKLOG_SIZE) {
            fprintf(stderr, "* (DELAYED ACCEPT) Error: reached connections LIMIT. Closing.\n");
            finalize_frontend_conn(conn);
            return;
        }
        // Otherwise start read
        uv_read_start((uv_stream_t*) conn, allocate_rx_buf, echo_read);
        
    } else {
        fprintf(stderr, "* Error (DELAYED ACCEPT): can't accept a new connection: %s\n",
            uv_strerror(result));
        finalize_frontend_conn(conn);
        return;
    }

}


void on_new_frontend_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }

    M01App* app = (M01App*) uv_default_loop()->data;

    // Schedule delayed accept if CONN LIMIT reached
    if (app->frontend_conn_pool.count() <= TCP_BACKLOG_SIZE) {
        fprintf(stderr, "* Error: CONN LIMIT. Scheduling DELAYED ACCEPT.\n");

        uv_timer_t* timer_handle;

        timer_handle = (uv_timer_t*) malloc(sizeof *timer_handle);
        uv_timer_init(uv_default_loop(), timer_handle);

        timer_handle->data = server;
        uv_timer_start(timer_handle, do_delayed_accept, 100, 0);
        return;
    }

    // Assign frontend conn_context
    M01FrontendConnContext* ctx = app->frontend_conn_pool.get_one();
    if (!ctx) {
        fprintf(stderr, "* Error: no CTX objects left in pool, returning.\n");
        return;
    }

    uv_tcp_t *conn = &ctx->connection;
 
    fprintf(stderr, "* Opening new conn: [%lld]\n", 
            (long long) conn);

    app->conn_count++;

    uv_tcp_init(uv_default_loop(), conn);
    int result = uv_accept(server, (uv_stream_t*) conn);
    if (result == 0) {
        // If no more conn context objects left in the pool
        if (app->frontend_conn_pool.count() < TCP_BACKLOG_SIZE) {
            fprintf(stderr, "* Error: reached connections LIMIT. Closing.\n");
            // Release this connection so that next time there is at least one
            // conn context to accept new conn
            finalize_frontend_conn(conn);
            return;
        }

        uv_read_start((uv_stream_t*) conn, allocate_rx_buf, echo_read);
        
    } else {
        fprintf(stderr, "* Error: can't accept a new connection: %s\n",
            uv_strerror(result));
        finalize_frontend_conn(conn);
        return;
    }
}



int main(int, char**) {
    //std:unique_ptr<M01App> = std::make_unique<M01App>();

    int result = -1;

    // Replace default memory management functions with custom
    // in order to reduce number of system mallocs
    // http://docs.libuv.org/en/v1.x/misc.html#c.uv_replace_allocator
    // result = uv_replace_allocator(m01_malloc, m01_realloc, m01_calloc, m01_free);
    // if (0 != result) {
    //     fprintf(stderr, "Fatal error while calling [uv_replace_allocator()]: %s\n",
    //         uv_strerror(result));
    //     return result;
    // }

    std::unique_ptr<M01App> m01app = std::make_unique<M01App>();
    std::cout << "** Starting qvtokens frontend ctl server ***\n";

    uv_tcp_t* server = new uv_tcp_t();
    

    // 1. Create socket
    // https://tech.flipkart.com/linux-tcp-so-reuseport-usage-and-implementation-6bfbf642885a
    uv_os_sock_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fprintf(stderr, "Error: can't create TCP socket");
        return 1;
    }

    // 2. Set sock options
    int sock_opt = 1;
    result = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sock_opt, sizeof sock_opt);

    if (result) {
        fprintf(stderr, "[setsockopt] error: %s\n", uv_strerror(result));
        return 1;
    }

     // 3. TCP init
    result = uv_tcp_init(uv_default_loop(), server);
    if (result) {
        fprintf(stderr, "[uv_tcp_init] error: %s\n", uv_strerror(result));
        return 1;
    }

    // 4. Open tcp socket
    result = uv_tcp_open(server, sock);
    if (result) {
        fprintf(stderr, "[uv_tcp_open] error: %s\n", uv_strerror(result));
        return 1;
    }

    
    // 5. Bind 
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &addr);

    result = uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
    if (result) {
        fprintf(stderr, "uv_tcp_bind error: %s\n", uv_strerror(result));
        return 1;
    }

    // 6. Listen for server (or connect for conn)
    result = uv_listen((uv_stream_t*) server, TCP_BACKLOG_SIZE, on_new_frontend_connection);
    if (result) {
        fprintf(stderr, "Listen error: %s\n", uv_strerror(result));
        return 1;
    }
    fprintf(stderr, "== Listening on port [%d]\n", DEFAULT_PORT);

    // Assign m01app to loop's data
    uv_default_loop()->data = m01app.get();

    // 7. Run the loop (blocks)
    result = uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    // 8. Handle graceful shutdown ==============================================
    fprintf(stderr, "==> Continuing graceful shutdown from main()\n");
    result = uv_loop_close(uv_default_loop());

    if (result) {

        fprintf(stderr, "failed to close libuv loop: %s\n",  uv_err_name(result));

        // Close all handles
        uv_walk(uv_default_loop(), on_close_active_handler, NULL);

        // Re-run loop to complete the shutdown
        result = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        if (result) {
            fprintf(stderr, "failed to restart loop: %s\n",  uv_err_name(result));
            return 1;
        } else {
            fprintf(stderr, "libuv loop restarted successfully!\n");
        }
        result = uv_loop_close(uv_default_loop());

        if (result) {
            fprintf(stderr, "FINALLY failed to close libuv loop: %s\n",  uv_err_name(result));
        } else {
            fprintf(stderr, "FINALLY libuv loop is closed successfully!\n");
        }

    } else {
        fprintf(stderr, "libuv loop is closed successfully!\n");
    }

    delete server;

    // For valgrind
    uv_loop_delete(uv_default_loop());

    m01_print_alloc_free_stats();
    
    return result;
}
