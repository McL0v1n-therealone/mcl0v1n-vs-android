# Architecture — Mcl0v1n vs android

How the pieces fit together on Android 4.4.4 / GT-I9300.

```
                       ANDROID 4.4.4 (KitKat) — GT-I9300
 ┌──────────────────────────────────────────────────────────────────┐
 │  init  (boot)                                                      │
 │    └─ service flash_recovery → /system/etc/install-recovery.sh     │
 │         ├─ mclovin_sepatch   (fix SELinux mode + su perms)         │
 │         └─ daemonsu --daemon ───────────────┐                      │
 │                                             ▼                      │
 │                                   ┌───────────────────┐            │
 │   unprivileged app (uid 10xxx)    │   mclovind (root) │            │
 │     calls:  su -c "id"            │  @mclovind socket │            │
 │        │                          └─────────▲─────────┘            │
 │        ▼                                    │ 1. SO_PEERCRED       │
 │   /system/xbin/su  ──connect──► abstract unix socket               │
 │        │  (client.c)               2. policy lookup (mclovin.policy)│
 │        │                           3. if unknown → ask manager      │
 │        │                                    │                      │
 │        │                       req-<pid> ▼  │ resp-<pid>           │
 │        │                 ┌───────────────────────────┐            │
 │        │                 │ Mcl0v1n manager (system app)│           │
 │        │                 │  RequestWatcherService      │           │
 │        │                 │   → SuRequestActivity (UI)  │           │
 │        │                 │   → PolicyDatabase (sqlite) │           │
 │        │                 │   → mclovin.policy (mirror) │           │
 │        │                 │   → mclovin.log (audit)     │           │
 │        │                 └───────────────────────────┘            │
 │        ▼  4. fork → setuid(target) → exec shell/command            │
 │   root process runs with your stdio (fd passing)                   │
 └──────────────────────────────────────────────────────────────────┘
```

## Why a daemon (and not plain setuid su)?

On Android 4.2 Samsung/Google tightened things; by 4.3/4.4 a plain setuid
binary is unreliable because:

- Zygote-spawned apps run in restricted process contexts; a setuid transition
  from an app domain can be blocked when SELinux is **enforcing**.
- `AID_` capabilities and `/proc/sys/kernel` restrictions make it cleaner to do
  the privileged `fork/exec` from a long-lived process that was started in the
  init context (which already holds full capabilities).

So `su` (client) is thin: it connects to **mclovind**, passes its stdio fds, and
relays the exit code. `mclovind` (same binary, different `argv[0]`) does the
authentication, policy, logging, and the actual privileged `exec`. This is the
same model SuperSU adopted on 4.3+.

## The single-binary trick

`native/su/` compiles to **one** static ELF. It behaves differently depending on
how it was invoked (`argv[0]`), set up by symlinks during install:

| Path | Role |
|------|------|
| `/system/xbin/su` | client (apps call this) |
| `/system/xbin/mclovind` → symlink to `su` | the daemon |
| `/system/xbin/daemonsu` → symlink to `su` | daemon launcher used by init hook |

Static linking avoids dependence on `/system/lib` versions that differ across
stock vs. CyanogenMod 11.

## Policy data flow

1. Manager writes rules to **SQLite** (`mclovin.db`) for its UI.
2. On every change it mirrors a dependency-free flat file **`mclovin.policy`**:
   `"<from_uid> <to_uid> ALLOW|DENY"` per line.
3. `mclovind` (C, no SQLite in NDK sysroot) reads the flat file in `policy.c`.
4. Unknown uid → daemon drops `req-<pid>`, manager's `FileObserver` pops the
   prompt, writes `resp-<pid>`, daemon reads the decision (**fail-closed** on
   timeout).
5. Every decision is appended to **`mclovin.log`**, shown by `LogActivity`.

## Persistence summary

Root persists across reboots because **all state lives on `/system`** (binaries
+ `install-recovery.sh`) which is mounted read-only at runtime but re-read every
boot, and the daemon is **relaunched by init** each boot. `BootReceiver` in the
app is only a secondary safety net; it cannot create root on its own.
