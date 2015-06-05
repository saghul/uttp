
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#if !defined(SO_REUSEPORT)
# error "Unsupported platform, SO_REUSEPORT is required."
#endif

#include "util.h"


static int uttp__cloexec(int fd, int set) {
    int flags;
    int r;

    do {
        r = fcntl(fd, F_GETFD);
    } while (r == -1 && errno == EINTR);

    if (r == -1) {
        return -errno;
    }

    /* Bail out now if already set/clear. */
    if (!!(r & FD_CLOEXEC) == !!set) {
        return 0;
    }

    if (set) {
        flags = r | FD_CLOEXEC;
    } else {
        flags = r & ~FD_CLOEXEC;
    }

    do {
        r = fcntl(fd, F_SETFD, flags);
    } while (r == -1 && errno == EINTR);

    if (r) {
        return -errno;
    }

    return 0;
}


/* create a TCP socket with CLOEXEC and set SO_REUSEADDR and SO_REUSEPORT
 * don't set NONBLOCK, since uv_tcp_open already does it */
int uttp_tcp_socket_create(int domain) {
    int fd;
    int r;
    int on;

    fd = socket(domain, SOCK_STREAM, 0);
    if (fd == -1) {
        return -errno;
    }

    r = uttp__cloexec(fd, 1);
    if (r != 0) {
        return r;
    }

    on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        return -errno;
    }

    on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on))) {
        return -errno;
    }

    return fd;
}
