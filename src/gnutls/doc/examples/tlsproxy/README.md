tlsproxy
========

`tlsproxy` is a TLS proxy written with GnuTLS. It is mostly designed as an
example of how to use asynchronous (non-blocking) I/O with GnuTLS. More
accurately, it was designed so I could learn how to do it. I think I've
got it right.

To that end, it's been divided up as follows:

* `crypto.c` does all the crypto, and `tlssession_mainloop()` does the hard work.
* `buffer.c` provides ring buffer support.
* `tlsproxy.c` deals with command line options and connecting sockets.

It can be used in two modes:

* Client mode (default). Listens on an unencrypted port, connects to
  an encrypted port.
* Server mode (run with `-s`). Listens on an encrypted port, connects to
  an unencrypted port.

Usage
=====

```
tlsproxy

Usage:
     tlsproxy [OPTIONS]

A TLS client or server proxy

Options:
     -c, --connect ADDRRESS    Connect to ADDRESS
     -l, --listen ADDRESS      Listen on ADDRESS
     -K, --key FILE            Use FILE as private key
     -C, --cert FILE           Use FILE as public key
     -A, --cacert FILE         Use FILE as public CA cert file
     -H, --hostname HOSTNAME   Use HOSTNAME to validate the CN of the peer
                               rather than hostname extracted from -C option
     -s, --server              Run the listen port encrypted rather than the
                               connect port
     -i, --insecure            Do not validate certificates
     -n, --nofork              Do not fork off (aids debugging); specify twice
                               to stop forking on accept as well
     -d, --debug               Turn on debugging
     -h, --help                Show this usage message
```

License
=======

MIT