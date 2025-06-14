#include <core/mnt.h>

#include <core/log.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

MountEntry *mnt_entry_head = NULL;

int ndm_mnt_dev(const char *devnode, const char *fstype,
                const char *mountpoint) {
    if (!devnode || !fstype || !mountpoint) {
        log_error("Invalid arguments: devnode=%s, fstype=%s, mountpoint=%s",
                  devnode ? devnode : "NULL", fstype ? fstype : "NULL",
                  mountpoint ? mountpoint : "NULL");
        return -1;
    }

    if (mkdir(mountpoint, 0755) != 0 && errno != EEXIST) {
        log_error("Error while running 'mkdir': %s", strerror(errno));
        return -5;
    }

    int res = mount(devnode, mountpoint, fstype, MS_RELATIME, "");
    if (res != 0) {
        log_error("Error while running 'mount': %s", strerror(errno));
        return -2;
    }

    log_info("Mounted %s on %s (fs=%s)", devnode, mountpoint, fstype);

    MountEntry *new_entry = malloc(sizeof(MountEntry));
    if (!new_entry) {
        log_error("Error while allocating memory: %s", strerror(errno));
        return -3;
    }

    new_entry->devnode    = strdup(devnode);
    new_entry->fstype     = strdup(fstype);
    new_entry->mountpoint = strdup(mountpoint);
    if (!new_entry->devnode || !new_entry->fstype || !new_entry->mountpoint) {
        log_error("Error while duplicating strings: %s", strerror(errno));
        free(new_entry->devnode);
        free(new_entry->fstype);
        free(new_entry->mountpoint);
        free(new_entry);
        return -4;
    }

    new_entry->next = NULL;

    if (mnt_entry_head == NULL) {
        mnt_entry_head = new_entry;
    } else {
        MountEntry *current = mnt_entry_head;
        while (current->next) {
            current = current->next;
        }
        current->next = new_entry;
    }

    return 0;
}

int ndm_mnt_unmount_all() {
    MountEntry *current = mnt_entry_head;
    while (current) {
        if (umount(current->mountpoint) != 0) {
            log_error("Error while unmounting %s: %s", current->mountpoint,
                      strerror(errno));
            return -1;
        }
        rmdir(current->mountpoint);
        log_info("Unmounted %s (was mounted at %s)", current->devnode,
                 current->mountpoint);
        MountEntry *to_free = current;
        current             = current->next;

        free(to_free->devnode);
        free(to_free->fstype);
        free(to_free->mountpoint);
        free(to_free);
    }
    mnt_entry_head = NULL;

    return 0;
}

MountEntry *ndm_mnt_get_entry_devnode(const char *devnode) {
    MountEntry *current = mnt_entry_head;
    while (current) {
        if (strcmp(current->devnode, devnode) == 0) {
            return current;
        }
        current = current->next;
    }
    log_warn("Could not find Mount Entry with the device node %s!", devnode);
    return NULL;
}

MountEntry *ndm_mnt_get_entry_mountpoint(const char *mountpoint) {
    MountEntry *current = mnt_entry_head;
    while (current) {
        if (strcmp(current->mountpoint, mountpoint) == 0) {
            return current;
        }
        current = current->next;
    }
    log_warn("Could not find Mount Point with the path %s!", mountpoint);
    return NULL;
}

void ndm_mnt_unmount(const char *mountpoint) {
    MountEntry *entry = ndm_mnt_get_entry_mountpoint(mountpoint);
    if (!entry) {
        log_error("Mount Entry with mountpoint %s not found!", mountpoint);
        return;
    }

    if (umount(entry->mountpoint) != 0) {
        log_error("Error while unmounting %s: %s", entry->mountpoint,
                  strerror(errno));
        return;
    }
    rmdir(entry->mountpoint);
    log_info("Unmounted %s (was mounted at %s)", entry->devnode,
             entry->mountpoint);

    if (mnt_entry_head == entry) {
        mnt_entry_head = entry->next;
    } else {
        MountEntry *current = mnt_entry_head;
        while (current && current->next != entry) {
            current = current->next;
        }
        if (current) {
            current->next = entry->next;
        }
    }

    free(entry->devnode);
    free(entry->fstype);
    free(entry->mountpoint);
    free(entry);
}
