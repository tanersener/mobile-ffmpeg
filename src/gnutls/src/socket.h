#include <gnutls/gnutls.h>
#include <gnutls/socket.h>

#define SOCKET_FLAG_UDP 1
#define SOCKET_FLAG_FASTOPEN (1<<1)
#define SOCKET_FLAG_STARTTLS (1<<2)
#define SOCKET_FLAG_RAW (1<<3) /* unencrypted */
#define SOCKET_FLAG_VERBOSE (1<<4)
#define SOCKET_FLAG_SKIP_INIT (1<<5)


typedef struct {
	int fd;
	gnutls_session_t session;
	int secure;
	char *hostname;
	const char *app_proto;
	char *ip;
	char *service;
	struct addrinfo *ptr;
	struct addrinfo *addr_info;
	int verbose;

	/* Needed for TCP Fast Open */
	struct sockaddr_storage connect_addr;
	socklen_t connect_addrlen;

	/* resumption data */
	gnutls_datum_t rdata;
} socket_st;

/* calling program must provide that */
extern gnutls_session_t init_tls_session(const char *host);
extern int do_handshake(socket_st * socket);

ssize_t socket_recv(const socket_st * socket, void *buffer,
		    int buffer_size);
ssize_t socket_recv_timeout(const socket_st * socket, void *buffer,
		    int buffer_size, unsigned ms);
ssize_t socket_send(const socket_st * socket, const void *buffer,
		    int buffer_size);
ssize_t socket_send_range(const socket_st * socket, const void *buffer,
			  int buffer_size, gnutls_range_st * range);
void
socket_open(socket_st * hd, const char *hostname, const char *service,
	    const char *app_proto, int flags, const char *msg, gnutls_datum_t *rdata);

void socket_bye(socket_st * socket, unsigned polite);

int service_to_port(const char *service, const char *proto);
const char *port_to_service(const char *sport, const char *proto);
int starttls_proto_to_port(const char *app_proto);
const char *starttls_proto_to_service(const char *app_proto);

void canonicalize_host(char *hostname, char *service, unsigned service_size);

#define CONNECT_MSG "Connecting to"
