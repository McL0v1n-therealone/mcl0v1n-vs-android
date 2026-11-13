/*
 * Mcl0v1n vs android - su.h
 * Shared definitions for the su client and the mclovind daemon.
 * Target: Android 4.4.4 (API 19), ARM, Samsung GT-I9300.
 */
#ifndef MCLOVIN_SU_H
#define MCLOVIN_SU_H

#include <sys/types.h>
#include <sys/un.h>
#include <limits.h>
#include <stdint.h>

#define MCLOVIN_VERSION       "1.0"
#define MCLOVIN_VERSION_CODE  100

/* The manager app's package + where its policy DB lives. */
#define MANAGER_PACKAGE   "com.mcl0v1n.superuser"
#define MANAGER_DATA_PATH "/data/data/" MANAGER_PACKAGE
#define POLICY_DB_PATH    MANAGER_DATA_PATH "/databases/mclovin.db"
#define LOG_PATH          MANAGER_DATA_PATH "/mclovin.log"

/* Abstract-namespace socket the daemon listens on (leading NUL added in code). */
#define DAEMON_SOCKET_NAME "mclovind"

/* Path the daemon writes a per-request socket into, used by the manager UI. */
#define REQUESTOR_SOCKET_DIR MANAGER_DATA_PATH "/mclovin_sockets"

/* install locations on /system */
#define SU_PATH        "/system/xbin/su"
#define DAEMON_PATH    "/system/xbin/mclovind"
#define DAEMONSU_PATH  "/system/xbin/daemonsu"

#define DEFAULT_SHELL  "/system/bin/sh"

/* Policy decisions stored/returned by the manager. */
typedef enum {
    ALLOW = 0,
    DENY  = 1,
    INTERACTIVE = 2   /* no cached rule -> ask the user */
} policy_t;

/* Context describing a single su invocation. */
struct su_request {
    uid_t  from_uid;          /* caller uid */
    pid_t  from_pid;          /* caller pid */
    uid_t  to_uid;            /* requested uid, default 0 (root) */
    gid_t  to_gid;            /* requested primary gid */
    int    login;             /* -l / --login */
    int    keepenv;           /* --preserve-environment */
    char  *shell;             /* -s / --shell */
    char  *command;           /* -c command string */
    char **argv;              /* remaining argv for exec */
    int    argc;
    char   from_name[PATH_MAX];   /* package name of caller, if resolvable */
};

/* daemon <-> client wire protocol opcodes (very small, length-prefixed) */
#define MSG_CMD      1  /* client -> daemon: full request payload */
#define MSG_RESULT   2  /* daemon -> client: exit code */
#define MSG_STDFD    3  /* fd passing for stdin/stdout/stderr */

/* helpers */
int   run_daemon(void);                       /* daemon.c */
int   connect_daemon(int argc, char **argv);  /* client.c */
policy_t database_check(struct su_request *req);     /* policy.c */
void  append_log(struct su_request *req, policy_t p);/* policy.c */
int   send_request_to_manager(struct su_request *req);/* policy.c */

#endif /* MCLOVIN_SU_H */
