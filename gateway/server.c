#include "server.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include "log.h"

#define MAX_EVENTS  128
#define MAX_ACCEPT  128
#define MAX_RB     4096
#define MAX_WB     8192

struct server {
        int listen_fd;
        int epoll_fd;
        fn_read_event read_event;
        fn_write_event write_event;
};

static int is_valid_fd(int fd)
{
        if (fcntl(fd, F_GETFD) == -1) {
                if (errno == EBADF)
                        return 0;
        }
        return 1;
}

static void accept_connection(struct server *srv)
{
        int cli;

        for (int i = 0; i < MAX_ACCEPT; i++) {
                cli = tcp_accept(srv->listen_fd, NULL);

                if (cli < 0) {
                        RETRY_ONCE_ENTR();

                        if (is_eagain())
                                break;
                }

                struct connection *conn;

                conn = connection_create(cli, srv->epoll_fd, MAX_RB, MAX_WB);

                if (!conn) {
                        close(cli);
                        continue;
                }

                struct epoll_event event = {
                        .events = EPOLLIN | EPOLLET,
                        .data.ptr = conn,
                };

                if (epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, cli, &event) < 0) {
                        connection_destroy(conn);
                        continue;
                }
        }
}

struct server *server_start(int port)
{
        struct server *srv;

        srv = calloc(1, sizeof(struct server));

        if (!srv)
                return NULL;

        /* start server */
        srv->listen_fd = tcp_create_listener(port);

        if (srv->listen_fd < 0)
                goto fatal;

        /* create epoll */
        srv->epoll_fd = epoll_create1(EPOLL_CLOEXEC);

        if (srv->epoll_fd < 0)
                goto fatal;

        /* add listen socket fd */
        struct epoll_event event = {
                .events = EPOLLIN,
                .data.fd = srv->listen_fd,
        };

        if (epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, srv->listen_fd, &event) < 0)
                goto fatal;

        logger_info("server start listening in port %d\n", port);

        return srv;

fatal:
        server_shutdown(srv);
        return NULL;
}

void server_shutdown(struct server *srv)
{
        if (is_valid_fd(srv->listen_fd))
                close(srv->listen_fd);

        if (is_valid_fd(srv->epoll_fd))
                close(srv->epoll_fd);

        free(srv);
}

void server_poll_events(struct server *srv)
{
        struct epoll_event events[MAX_EVENTS];
        int n = epoll_wait(srv->epoll_fd, events, MAX_EVENTS, -1);

        if (n <= 0)
                return;

        for (int i = 0; i < n; i++) {
                struct epoll_event *cev = &events[i];

                if (cev->data.fd == srv->listen_fd) {
                        accept_connection(srv);
                        continue;
                }

                if (cev->events & EPOLLIN && srv->read_event) {
                        srv->read_event((struct connection *) cev->data.ptr);
                        continue;
                }

                if (cev->events & EPOLLOUT && srv->write_event) {
                        srv->write_event((struct connection *) cev->data.ptr);
                        continue;
                }
        }
}

void server_set_read_event(struct server *srv, fn_read_event read_event)
{
        (srv)->read_event = (read_event);
}

void server_set_write_event(struct server *srv, fn_write_event write_event)
{
        (srv)->write_event = (write_event);
}