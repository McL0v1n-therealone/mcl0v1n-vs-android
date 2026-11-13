/*
 * Mcl0v1n vs android - su.c
 * The `su` client entry point.
 *
 * Flow:
 *   1. Parse args (su-compatible: -c, -s, -l, -, user).
 *   2. If we are already root and called by the daemon, just exec.
 *   3. Otherwise connect to mclovind (the persistent daemon started by init),
 *      which performs the privileged work in a root context. This is the
 *      "daemon mode" model used on 4.3+/KitKat where setuid alone is not
 *      enough under SELinux-enforcing kernels.
 *
 * Target: Android 4.4.4, ARM, GT-I9300.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>

#include "su.h"

static void usage(int status) {
    FILE *s = (status == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(s,
        "Mcl0v1n vs android su v%s\n"
        "Usage: su [options] [--] [-] [LOGIN] [ARGS...]\n\n"
        "  -c, --command COMMAND         pass COMMAND to the invoked shell\n"
        "  -s, --shell SHELL             use SHELL instead of " DEFAULT_SHELL "\n"
        "  -l, --login                   make the shell a login shell\n"
        "  -p, --preserve-environment    do not reset environment variables\n"
        "  -v, --version                 print version and exit\n"
        "  -h, --help                    print this help\n",
        MCLOVIN_VERSION);
    exit(status);
}

/* Determine the uid we were called from, resolve name for logging. */
static void fill_caller(struct su_request *req) {
    req->from_uid = getuid();
    req->from_pid = getppid();
    req->from_name[0] = '\0';
    /* The daemon does the authoritative package lookup via /proc + PackageManager;
       here we leave a best-effort placeholder. */
}

int main(int argc, char **argv) {
    struct su_request req;
    memset(&req, 0, sizeof(req));
    req.to_uid = 0;
    req.to_gid = 0;
    req.shell  = DEFAULT_SHELL;

    /* If invoked as the daemon ("mclovind" / "su --daemon"), run the daemon. */
    char *base = strrchr(argv[0], '/');
    base = base ? base + 1 : argv[0];
    if (strcmp(base, "mclovind") == 0 || strcmp(base, "daemonsu") == 0)
        return run_daemon();

    static struct option opts[] = {
        {"command", required_argument, 0, 'c'},
        {"shell",   required_argument, 0, 's'},
        {"login",   no_argument,       0, 'l'},
        {"preserve-environment", no_argument, 0, 'p'},
        {"daemon",  no_argument,       0, 'D'},
        {"version", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {0,0,0,0}
    };

    int c, idx;
    while ((c = getopt_long(argc, argv, "+c:s:lpvhD", opts, &idx)) != -1) {
        switch (c) {
            case 'c': req.command = optarg; break;
            case 's': req.shell = optarg; break;
            case 'l': req.login = 1; break;
            case 'p': req.keepenv = 1; break;
            case 'D': return run_daemon();
            case 'v': printf("%s (Mcl0v1n vs android)\n", MCLOVIN_VERSION); return 0;
            case 'h': usage(EXIT_SUCCESS); break;
            default:  usage(EXIT_FAILURE); break;
        }
    }

    /* Optional target user (name or uid) */
    if (optind < argc && strcmp(argv[optind], "-") != 0) {
        struct passwd *pw = getpwnam(argv[optind]);
        if (pw) {
            req.to_uid = pw->pw_uid;
            req.to_gid = pw->pw_gid;
        } else {
            char *end;
            long uid = strtol(argv[optind], &end, 10);
            if (*end == '\0') req.to_uid = (uid_t)uid;
        }
        optind++;
    }

    req.argv = &argv[optind];
    req.argc = argc - optind;

    fill_caller(&req);

    /* Root/shell callers from the daemon context are trusted and go straight
       through; everyone else is routed through the daemon for policy + privilege. */
    return connect_daemon(argc, argv);
}
