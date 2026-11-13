/*
 * Mcl0v1n vs android - policy.c
 * Policy lookup, interactive prompt bridging, and audit logging.
 *
 * Design note (KitKat-era, no libsqlite in NDK sysroot):
 * The manager app maintains the authoritative rules in its SQLite DB
 * (mclovin.db) for its UI, and ALSO mirrors a flat, line-based policy file at
 *   /data/data/com.mcl0v1n.superuser/mclovin.policy
 * which the daemon can parse with zero dependencies. Each line is:
 *   <from_uid> <to_uid> <ALLOW|DENY>
 * The manager rewrites this file whenever a rule is added/revoked.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "su.h"

#define POLICY_FILE MANAGER_DATA_PATH "/mclovin.policy"

policy_t database_check(struct su_request *req) {
    FILE *f = fopen(POLICY_FILE, "r");
    if (!f) return INTERACTIVE;   /* no rules yet -> ask */

    char line[256];
    policy_t result = INTERACTIVE;
    while (fgets(line, sizeof(line), f)) {
        unsigned int fu, tu;
        char verb[16];
        if (sscanf(line, "%u %u %15s", &fu, &tu, verb) == 3) {
            if (fu == req->from_uid && tu == req->to_uid) {
                if (!strcasecmp(verb, "ALLOW")) result = ALLOW;
                else if (!strcasecmp(verb, "DENY")) result = DENY;
                break;
            }
        }
    }
    fclose(f);
    return result;
}

/*
 * Ask the manager app for an interactive decision. We drop a request file the
 * manager is watching (via a FileObserver) and block on a response socket.
 * Timeout defaults to DENY (fail-closed) so a missing/crashed manager can never
 * silently grant root.
 */
int send_request_to_manager(struct su_request *req) {
    /* Ensure the handoff dir exists (daemon is root, so this is fine). */
    mkdir(REQUESTOR_SOCKET_DIR, 0771);

    char reqpath[PATH_MAX];
    snprintf(reqpath, sizeof(reqpath), "%s/req-%d", REQUESTOR_SOCKET_DIR,
             (int)req->from_pid);

    FILE *f = fopen(reqpath, "w");
    if (!f) return DENY;
    fprintf(f, "from_uid=%u\nto_uid=%u\npid=%d\ncaller=%s\n",
            req->from_uid, req->to_uid, (int)req->from_pid, req->from_name);
    fclose(f);
    chmod(reqpath, 0664);

    /* Poll for the decision file the manager writes back. */
    char resppath[PATH_MAX];
    snprintf(resppath, sizeof(resppath), "%s/resp-%d", REQUESTOR_SOCKET_DIR,
             (int)req->from_pid);

    int decision = DENY;
    for (int i = 0; i < 300; i++) {   /* up to ~30s @ 100ms */
        FILE *r = fopen(resppath, "r");
        if (r) {
            char verb[16] = {0};
            if (fscanf(r, "%15s", verb) == 1) {
                if (!strcasecmp(verb, "ALLOW")) decision = ALLOW;
                else decision = DENY;
            }
            fclose(r);
            unlink(resppath);
            unlink(reqpath);
            return decision;
        }
        usleep(100 * 1000);
    }
    unlink(reqpath);
    return DENY;   /* fail-closed on timeout */
}

void append_log(struct su_request *req, policy_t p) {
    FILE *f = fopen(LOG_PATH, "a");
    if (!f) return;
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    const char *verb = (p == ALLOW) ? "ALLOW" :
                       (p == DENY)  ? "DENY"  : "PROMPT";
    fprintf(f, "%s uid=%u->%u pid=%d %s %s\n",
            ts, req->from_uid, req->to_uid, (int)req->from_pid,
            verb, req->from_name);
    fclose(f);
}
