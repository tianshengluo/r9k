#ifndef SERVER_H_
#define SERVER_H_

#include <sys/epoll.h>

#include "socket.h"
#include "conn.h"

typedef void (*fn_read_event) (struct connection *);
typedef void (*fn_write_event) (struct connection *);

struct server;

struct server *server_start(int port);
void server_shutdown(struct server *srv);
void server_poll_events(struct server *srv);
void server_set_read_event(struct server *srv, fn_read_event read_event);
void server_set_write_event(struct server *srv, fn_write_event write_event);

#endif /* SERVER_H_ */
