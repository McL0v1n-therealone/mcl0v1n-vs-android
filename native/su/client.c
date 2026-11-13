/*
 * Mcl0v1n vs android - client.c
 * Connects the unprivileged `su` invocation to the persistent mclovind daemon
 * over an abstract-namespace unix socket, passes stdio fds, and relays the
 * exit code back.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "su.h"

/* Build an abstract-namespace sockaddr (first byte NUL). */
static socklen_t make_addr(struct sockaddr_un *addr, const char *name) {
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_LOCAL;
    addr->sun_path[0] = '\0';
    strncpy(addr->sun_path + 1, name, sizeof(addr->sun_path) - 2);
    return offsetof(struct sockaddr_un, sun_path) + 1 + strlen(name);
}

/* Send one fd over the control message channel. */
static int send_fd(int sockfd, int fd) {
    struct msghdr msg;
    struct iovec iov;
    char buf[CMSG_SPACE(sizeof(int))];
    char dummy = 'F';

    memset(&msg, 0, sizeof(msg));
    memset(buf, 0, sizeof(buf));
    iov.iov_base = &dummy;
    iov.iov_len  = 1;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
    *((int *)CMSG_DATA(cmsg)) = fd;

    return sendmsg(sockfd, &msg, 0) < 0 ? -1 : 0;
}

static int write_str(int fd, const char *s) {
    uint32_t len = s ? (uint32_t)strlen(s) : 0;
    if (write(fd, &len, sizeof(len)) != sizeof(len)) return -1;
    if (len && write(fd, s, len) != (ssize_t)len) return -1;
    return 0;
}

int connect_daemon(int argc, char **argv) {
    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    socklen_t len = make_addr(&addr, DAEMON_SOCKET_NAME);
    if (connect(fd, (struct sockaddr *)&addr, len) < 0) {
        fprintf(stderr,
            "mclovin: cannot reach daemon (is mclovind running?)\n"
            "         try: /system/xbin/daemonsu --daemon &\n");
        close(fd);
        return 1;
    }

    /* Hand over our stdio so the spawned root process shares this terminal. */
    send_fd(fd, STDIN_FILENO);
    send_fd(fd, STDOUT_FILENO);
    send_fd(fd, STDERR_FILENO);

    /* Send cwd + env size + argv so the daemon can reconstruct the exec. */
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "/");
    write_str(fd, cwd);

    uint32_t n = (uint32_t)argc;
    write(fd, &n, sizeof(n));
    for (int i = 0; i < argc; i++) write_str(fd, argv[i]);

    /* Wait for the daemon's final exit code. */
    int32_t exit_code = 1;
    if (read(fd, &exit_code, sizeof(exit_code)) != sizeof(exit_code))
        exit_code = 1;

    close(fd);
    return exit_code;
}
