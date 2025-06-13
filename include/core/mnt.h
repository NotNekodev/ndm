#ifndef MNT_H
#define MNT_H

typedef struct _MountEntry {
    char *devnode;
    char *fstype;
    char *mountpoint;

    struct _MountEntry *next;
} MountEntry;

extern MountEntry *mnt_entry_head;

int ndm_mnt_dev(const char *devnode, const char *fstype,
                const char *mountpoint);
int ndm_mnt_unmount_all();

MountEntry *ndm_mnt_get_entry_devnode(const char *devnode);
MountEntry *ndm_mnt_get_entry_mountpoint(const char *mountpoint);
void ndm_mnt_unmount(const char *mountpoint);

#endif // MNT_H
