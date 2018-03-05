#define min(x,y) ((x)<(y)?(x):(y))

extern const char *side;

#define HANDSHAKE_EXPECT(c, s, clierr, serverr) \
  sret = cret = GNUTLS_E_AGAIN; \
  do \
    { \
      if (cret == GNUTLS_E_AGAIN) \
	{ \
	  side = "client"; \
	  cret = gnutls_handshake (c); \
	  if (cret == GNUTLS_E_INTERRUPTED) cret = GNUTLS_E_AGAIN; \
	} \
      if (sret == GNUTLS_E_AGAIN) \
	{ \
	  side = "server"; \
	  sret = gnutls_handshake (s); \
	  if (sret == GNUTLS_E_INTERRUPTED) sret = GNUTLS_E_AGAIN; \
	} \
    } \
  while ((cret == GNUTLS_E_AGAIN || (cret == 0 && sret == GNUTLS_E_AGAIN)) && (sret == GNUTLS_E_AGAIN || (sret == 0 && cret == GNUTLS_E_AGAIN))); \
  if (cret != clierr || sret != serverr) \
    { \
      fprintf(stderr, "client[%d]: %s\n", cret, gnutls_strerror(cret)); \
      fprintf(stderr, "server[%d]: %s\n", sret, gnutls_strerror(sret)); \
      fail("%s:%d: Handshake failed\n", __func__, __LINE__); \
      exit(1); \
    }

#define HANDSHAKE(c, s) \
  HANDSHAKE_EXPECT(c,s,0,0)

#define HANDSHAKE_DTLS_EXPECT(c, s, clierr, serverr) \
  sret = cret = GNUTLS_E_AGAIN; \
  do \
    { \
      if (cret == GNUTLS_E_LARGE_PACKET) \
	{ \
	  unsigned int mtu = gnutls_dtls_get_mtu(s); \
	  gnutls_dtls_set_mtu(s, mtu/2); \
	} \
      if (cret < 0 && gnutls_error_is_fatal(cret) == 0) \
	{ \
	  side = "client"; \
	  cret = gnutls_handshake (c); \
	} \
      if (sret == GNUTLS_E_LARGE_PACKET) \
	{ \
	  unsigned int mtu = gnutls_dtls_get_mtu(s); \
	  gnutls_dtls_set_mtu(s, mtu/2); \
	} \
      if (sret < 0 && gnutls_error_is_fatal(sret) == 0) \
	{ \
	  side = "server"; \
	  sret = gnutls_handshake (s); \
	} \
    } \
  while (((gnutls_error_is_fatal(cret) == 0 && gnutls_error_is_fatal(sret) == 0)) && (cret < 0 || sret < 0)); \
  if (cret != clierr || sret != serverr) \
    { \
      fprintf(stderr, "client: %s\n", gnutls_strerror(cret)); \
      fprintf(stderr, "server: %s\n", gnutls_strerror(sret)); \
      fail("%s:%d: Handshake failed\n", __func__, __LINE__); \
      exit(1); \
    }

#define HANDSHAKE_DTLS(c, s) \
  HANDSHAKE_DTLS_EXPECT(c,s,0,0)

#define HANDSHAKE(c, s) \
  HANDSHAKE_EXPECT(c,s,0,0)

#define TRANSFER2(c, s, msg, msglen, buf, buflen, retry_send_with_null) \
  side = "client"; \
  ret = record_send_loop (c, msg, msglen, retry_send_with_null); \
  \
  if (ret < 0) fail ("client send error: %s\n", gnutls_strerror (ret)); \
  \
  do \
    { \
      do \
	{ \
	  side = "server"; \
	  ret = gnutls_record_recv (s, buf, buflen); \
	} \
      while(ret == GNUTLS_E_AGAIN); \
      if (ret == 0) \
	fail ("server: didn't receive any data\n"); \
      else if (ret < 0) \
	{ \
	  fail ("server: error: %s\n", gnutls_strerror (ret)); \
	} \
      else \
	{ \
	  transferred += ret; \
	} \
      side = "server"; \
      ns = record_send_loop (server, msg, msglen, retry_send_with_null); \
      if (ns < 0) fail ("server send error: %s\n", gnutls_strerror (ret)); \
      do \
	{ \
	  side = "client"; \
	  ret = gnutls_record_recv (client, buf, buflen); \
	} \
      while(ret == GNUTLS_E_AGAIN); \
      if (ret == 0) \
	{ \
	  fail ("client: Peer has closed the TLS connection\n"); \
	} \
      else if (ret < 0) \
	{ \
	  if (debug) \
	    fputs ("!", stdout); \
	  fail ("client: Error: %s\n", gnutls_strerror (ret)); \
	} \
      else \
	{ \
	  if (msglen != ret || memcmp (buf, msg, msglen) != 0) \
	    { \
	      fail ("client: Transmitted data do not match\n"); \
	    } \
	  /* echo back */ \
	  side = "client"; \
	  ns = record_send_loop (client, buf, msglen, retry_send_with_null); \
	  if (ns < 0) fail ("client send error: %s\n", gnutls_strerror (ret)); \
	  transferred += ret; \
	  if (debug) \
	    fputs (".", stdout); \
	} \
    } \
  while (transferred < 70000)

#define TRANSFER(c, s, msg, msglen, buf, buflen) \
  TRANSFER2(c, s, msg, msglen, buf, buflen, 0); \
  TRANSFER2(c, s, msg, msglen, buf, buflen, 1)

static char to_server[64 * 1024];
static size_t to_server_len = 0;

static char to_client[64 * 1024];
static size_t to_client_len = 0;

#ifdef RANDOMIZE
#define RETURN_RND_EAGAIN(session) \
  static unsigned char rnd = 0; \
  if (rnd++ % 2 == 0) \
    { \
      gnutls_transport_set_errno (session, EAGAIN); \
      return -1; \
    }
#else
#define RETURN_RND_EAGAIN(session)
#endif

#ifndef IGNORE_PUSH
static ssize_t
client_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	size_t newlen;
	RETURN_RND_EAGAIN(tr);

	len = min(len, sizeof(to_server) - to_server_len);

	newlen = to_server_len + len;
	memcpy(to_server + to_server_len, data, len);
	to_server_len = newlen;
#ifdef EAGAIN_DEBUG
	fprintf(stderr, "eagain: pushed %d bytes to server (avail: %d)\n",
		(int) len, (int) to_server_len);
#endif
	return len;
}

#endif

static ssize_t
client_pull(gnutls_transport_ptr_t tr, void *data, size_t len)
{
	RETURN_RND_EAGAIN(tr);

	if (to_client_len == 0) {
#ifdef EAGAIN_DEBUG
		fprintf(stderr,
			"eagain: Not enough data by server (asked for: %d, have: %d)\n",
			(int) len, (int) to_client_len);
#endif
		gnutls_transport_set_errno((gnutls_session_t) tr, EAGAIN);
		return -1;
	}

	len = min(len, to_client_len);

	memcpy(data, to_client, len);

	memmove(to_client, to_client + len, to_client_len - len);
	to_client_len -= len;
#ifdef EAGAIN_DEBUG
	fprintf(stderr, "eagain: pulled %d bytes by client (avail: %d)\n",
		(int) len, (int) to_client_len);
#endif
	return len;
}

static ssize_t
server_pull(gnutls_transport_ptr_t tr, void *data, size_t len)
{
	//success ("server_pull len %d has %d\n", len, to_server_len);
	RETURN_RND_EAGAIN(tr);

	if (to_server_len == 0) {
#ifdef EAGAIN_DEBUG
		fprintf(stderr,
			"eagain: Not enough data by client (asked for: %d, have: %d)\n",
			(int) len, (int) to_server_len);
#endif
		gnutls_transport_set_errno((gnutls_session_t) tr, EAGAIN);
		return -1;
	}

	len = min(len, to_server_len);
#ifdef EAGAIN_DEBUG
	fprintf(stderr, "eagain: pulled %d bytes by server (avail: %d)\n",
		(int) len, (int) to_server_len);
#endif
	memcpy(data, to_server, len);

	memmove(to_server, to_server + len, to_server_len - len);
	to_server_len -= len;

	return len;
}

#ifndef IGNORE_PUSH
static ssize_t
server_push(gnutls_transport_ptr_t tr, const void *data, size_t len)
{
	size_t newlen;
	RETURN_RND_EAGAIN(tr);

//  hexprint (data, len);

	len = min(len, sizeof(to_client) - to_client_len);

	newlen = to_client_len + len;
	memcpy(to_client + to_client_len, data, len);
	to_client_len = newlen;
#ifdef EAGAIN_DEBUG
	fprintf(stderr, "eagain: pushed %d bytes to client (avail: %d)\n",
		(int) len, (int) to_client_len);
#endif


#ifdef SERVER_PUSH_ADD
	SERVER_PUSH_ADD
#endif

	return len;
}

#endif

/* inline is used to avoid a gcc warning if used in mini-eagain */
inline static int server_pull_timeout_func(gnutls_transport_ptr_t ptr,
					   unsigned int ms)
{
	int ret;

	if (to_server_len > 0)
		ret = 1;	/* available data */
	else
		ret = 0;	/* timeout */

#ifdef EAGAIN_DEBUG
	fprintf(stderr,
		"eagain: server_pull_timeout: %d (avail: cli %d, serv %d)\n",
		ret, (int) to_client_len, (int) to_server_len);
#endif

	return ret;
}

inline static int client_pull_timeout_func(gnutls_transport_ptr_t ptr,
					   unsigned int ms)
{
	int ret;

	if (to_client_len > 0)
		ret = 1;
	else
		ret = 0;

#ifdef EAGAIN_DEBUG
	fprintf(stderr,
		"eagain: client_pull_timeout: %d (avail: cli %d, serv %d)\n",
		ret, (int) to_client_len, (int) to_server_len);
#endif

	return ret;
}

inline static void reset_buffers(void)
{
	to_server_len = 0;
	to_client_len = 0;
}

inline static int record_send_loop(gnutls_session_t session,
				   const void *data, size_t sizeofdata,
				   int use_null_on_retry)
{
	int ret;
	const void *retry_data;
	size_t retry_sizeofdata;

	if (use_null_on_retry) {
		retry_data = 0;
		retry_sizeofdata = 0;
	} else {
		retry_data = data;
		retry_sizeofdata = sizeofdata;
	}

	ret = gnutls_record_send(session, data, sizeofdata);
	while (ret == GNUTLS_E_AGAIN) {
		ret =
		    gnutls_record_send(session, retry_data,
					retry_sizeofdata);
	}

	return ret;
}
