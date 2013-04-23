#ifndef AROS_COMPAT_H
#define AROS_COMPAT_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/mount.h>
#include <proto/socket.h>
#include <proto/exec.h>
#include <sys/uio.h>

#define statvfs statfs
#define ioctl IoctlSocket
#define close CloseSocket

#define writev aros_writev
#define readv aros_readv
#define inet_pton aros_inet_pton
#define freeaddrinfo aros_freeaddrinfo
#define getnameinfo aros_getnameinfo
#define getaddrinfo aros_getaddrinfo

#define SOL_TCP IPPROTO_TCP

extern struct Library * SocketBase;

void aros_init_socket(void);

#define POLLIN      0x0001    /* There is data to read */
#define POLLPRI     0x0002    /* There is urgent data to read */
#define POLLOUT     0x0004    /* Writing now will not block */
#define POLLERR     0x0008    /* Error condition */
#define POLLHUP     0x0010    /* Hung up */
#define POLLNVAL    0x0020    /* Invalid request: fd not open */

struct pollfd {
    int fd;           /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
};

#define poll(x, y, z)        aros_poll(x, y, z)

#endif
