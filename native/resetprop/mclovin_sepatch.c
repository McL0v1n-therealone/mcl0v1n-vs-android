/*
 * Mcl0v1n vs android - mclovin_sepatch
 * A tiny boot helper run by the init hook BEFORE launching mclovind.
 *
 * On the GT-I9300 Samsung stock 4.4.4 ships SELinux in *permissive* mode, so
 * root generally works without policy edits. However some custom KitKat ROMs
 * (CyanogenMod 11) run *enforcing*. To keep the daemon able to transition into
 * domains and relabel its socket, this helper:
 *
 *   1. If the kernel is enforcing AND we still have the privilege, drop to
 *      permissive (writes "0" to /sys/fs/selinux/enforce). This mirrors what
 *      older SuperSU did on 4.4 before "sepolicy injection" existed.
 *   2. Ensures /system/xbin/su has the right mode/owner (root:root 06755).
 *
 * This is intentionally dependency-free and static-linkable.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static int read_enforce(void) {
    int fd = open("/sys/fs/selinux/enforce", O_RDONLY);
    if (fd < 0) fd = open("/selinux/enforce", O_RDONLY);
    if (fd < 0) return -1;
    char c = '0';
    read(fd, &c, 1);
    close(fd);
    return c == '1' ? 1 : 0;
}

static void set_permissive(void) {
    int fd = open("/sys/fs/selinux/enforce", O_WRONLY);
    if (fd < 0) fd = open("/selinux/enforce", O_WRONLY);
    if (fd < 0) return;
    write(fd, "0", 1);
    close(fd);
}

int main(void) {
    if (read_enforce() == 1) {
        fprintf(stderr, "mclovin_sepatch: SELinux enforcing -> permissive\n");
        set_permissive();
    }
    /* enforce correct perms on the su binary each boot (survives OTA fixups). */
    chmod("/system/xbin/su", 06755);
    chown("/system/xbin/su", 0, 0);
    return 0;
}
