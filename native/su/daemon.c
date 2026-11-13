/*
 * Mcl0v1n vs android - daemon.c
 * The persistent root daemon (mclovind).
 *
 * It is started at boot by init (see flashable/mclovin/common/mclovin-boot.sh
 * -> install-recovery.sh / init.d hook) while the process is still in a root,
 * permissive-capable context. It listens on an abstract unix socket. When an
 * unprivileged `su` client connects:
 *
 *   1. Read caller credentials via SO_PEERCRED (authoritative uid/pid).
 *   2. Resolve the caller package name from /proc/<pid>/cmdline.
 *   3. Consult the policy DB (ALLOW/DENY/INTERACTIVE).
 *   4. If INTERACTIVE, poke the manager app and wait for the user's decision.
 *   5. Log the event.
 *   6. If allowed, fork a child, setgid/setuid to the target, restore the
 *      client's stdio fds, and exec the requested shell/command.
 *   7. Relay the child's exit code back to the client.
 *
 * Because the binary + daemon live on /system and init relaunches the daemon
 * every boot, root is PERSISTENT across reboots.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/prctl.h>

#include "su.h"

static socklen_t make_addr(struct sockaddr_un *addr, const char *name) {
    memset(addr, 0, sizeof(*addr));
    addr->sun_family = AF_LOCAL;
    addr->sun_path[0] = '\0';
    strncpy(addr->sun_path + 1, name, sizeof(addr->sun_path) - 2);
    return offsetof(struct sockaddr_un, sun_path) + 1 + strlen(name);
}

static int recv_fd(int sockfd) {
    struct msghdr msg;
    struct iovec iov;
    char buf[CMSG_SPACE(sizeof(int))];
    char dummy;

    memset(&msg, 0, sizeof(msg));
    iov.iov_base = &dummy; iov.iov_len = 1;
    msg.msg_iov = &iov; msg.msg_iovlen = 1;
    msg.msg_control = buf; msg.msg_controllen = sizeof(buf);

    if (recvmsg(sockfd, &msg, 0) <= 0) return -1;
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg || cmsg->cmsg_type != SCM_RIGHTS) return -1;
    return *((int *)CMSG_DATA(cmsg));
}

static char *read_str(int fd) {
    uint32_t len;
    if (read(fd, &len, sizeof(len)) != sizeof(len)) return NULL;
    char *s = malloc(len + 1);
    if (!s) return NULL;
    if (len && read(fd, s, len) != (ssize_t)len) { free(s); return NULL; }
    s[len] = '\0';
    return s;
}

/* Resolve the caller's package/exe name from /proc for policy + logging. */
static void resolve_name(pid_t pid, char *out, size_t n) {
    char path[64], buf[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    int fd = open(path, O_RDONLY);
    out[0] = '\0';
    if (fd >= 0) {
        ssize_t r = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (r > 0) { buf[r] = '\0'; strncpy(out, buf, n - 1); out[n-1] = '\0'; }
    }
    if (!out[0]) strncpy(out, "unknown", n);
}

/* Handle a single accepted client connection. Runs in a forked child. */
static void handle_client(int client) {
    struct ucred cr;
    socklen_t crlen = sizeof(cr);
    if (getsockopt(client, SOL_SOCKET, SO_PEERCRED, &cr, &crlen) < 0) {
        close(client); _exit(1);
    }

    struct su_request req;
    memset(&req, 0, sizeof(req));
    req.from_uid = cr.uid;
    req.from_pid = cr.pid;
    req.to_uid = 0; req.to_gid = 0;
    req.shell = DEFAULT_SHELL;
    resolve_name(cr.pid, req.from_name, sizeof(req.from_name));

    /* Receive the 3 stdio fds. */
    int in  = recv_fd(client);
    int out = recv_fd(client);
    int err = recv_fd(client);

    char *cwd = read_str(client);
    uint32_t argc = 0;
    read(client, &argc, sizeof(argc));
    char **cargv = calloc(argc + 1, sizeof(char *));
    for (uint32_t i = 0; i < argc; i++) cargv[i] = read_str(client);

    /* ---- Policy decision ---- */
    /* root/shell (adb) are implicitly trusted so the manager can bootstrap. */
    policy_t p;
    if (req.from_uid == 0 || req.from_uid == 2000 /*AID_SHELL*/)
        p = ALLOW;
    else
        p = database_check(&req);

    if (p == INTERACTIVE)
        p = (policy_t)send_request_to_manager(&req);

    append_log(&req, p);

    if (p != ALLOW) {
        int32_t code = 1;
        write(client, &code, sizeof(code));
        close(client); _exit(0);
    }

    /* ---- Privileged exec ---- */
    pid_t child = fork();
    if (child == 0) {
        /* child: become target user, attach client's stdio, exec */
        if (in  >= 0) dup2(in,  STDIN_FILENO);
        if (out >= 0) dup2(out, STDOUT_FILENO);
        if (err >= 0) dup2(err, STDERR_FILENO);

        setgroups(0, NULL);
        setgid(req.to_gid);
        setuid(req.to_uid);

        if (cwd && *cwd) chdir(cwd);

        /* Build argv: if no explicit argv, start the shell. */
        char *shell = req.shell ? req.shell : DEFAULT_SHELL;
        if (argc == 0 || (argc == 1)) {
            char *sh_argv[] = { shell, NULL };
            execv(shell, sh_argv);
        } else {
            /* cargv[0] is "su"; forward the remaining args to the shell -c
               or exec directly. For simplicity exec the shell with -c of the
               joined remainder when a command was requested. */
            execv(shell, cargv);
        }
        _exit(127); /* exec failed */
    }

    int status = 0;
    waitpid(child, &status, 0);
    int32_t code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    write(client, &code, sizeof(code));
    close(client);
    _exit(0);
}

int run_daemon(void) {
    /* We expect to already be root (started by init). Verify loudly. */
    if (getuid() != 0) {
        fprintf(stderr, "mclovind: must run as root (uid=%d)\n", getuid());
        /* continue anyway; on some kernels init launches us post-setuid */
    }

    umask(077);
    /* auto-reap handler children. Use sigaction (signal() pulls in bsd_signal
       which isn't available when static-linking against API-19 bionic). */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);
    prctl(PR_SET_NAME, "mclovind", 0, 0, 0);

    int sfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    socklen_t len = make_addr(&addr, DAEMON_SOCKET_NAME);
    if (bind(sfd, (struct sockaddr *)&addr, len) < 0) {
        perror("bind (another mclovind already running?)");
        return 1;
    }
    if (listen(sfd, 16) < 0) { perror("listen"); return 1; }

    fprintf(stderr, "mclovind v%s listening on @%s\n",
            MCLOVIN_VERSION, DAEMON_SOCKET_NAME);

    for (;;) {
        int client = accept(sfd, NULL, NULL);
        if (client < 0) { if (errno == EINTR) continue; break; }
        pid_t pid = fork();
        if (pid == 0) { close(sfd); handle_client(client); _exit(0); }
        close(client);
    }
    close(sfd);
    return 0;
}
