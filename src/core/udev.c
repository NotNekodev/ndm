#include <core/udev.h>

#include <blkid/blkid.h>
#include <core/log.h>
#include <core/mnt.h>
#include <errno.h>
#include <libudev.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct udev *udev                = NULL;
struct udev_monitor *monitor     = NULL;
struct udev_enumerate *enumerate = NULL;
int mon_fd                       = -1;
pthread_t usb_udev_thread;
int i           = 0;
int usb_counter = 0;

int ndm_udev_init_usb() {
    if (!udev) {
        udev = udev_new();
        if (!udev) {
            log_error("Failed to retrieve udev!");
            return -1;
        }
    }

    monitor = udev_monitor_new_from_netlink(udev, "udev");
    if (!monitor) {
        log_error("Failed to retrieve monitor!");
        udev_unref(udev);
        return -2;
    }

    udev_monitor_filter_add_match_subsystem_devtype(monitor, "block", NULL);
    udev_monitor_enable_receiving(monitor);

    mon_fd = udev_monitor_get_fd(monitor);

    if (mon_fd < 0) {
        log_error("Invalid Monitor File Descriptor (%d)!", mon_fd);
        udev_monitor_unref(monitor);
        udev_unref(udev);
        return -3;
    }

    if (pthread_create(&usb_udev_thread, NULL, (void *)ndm_udev_thread_usb,
                       NULL) != 0) {
        log_error("Could not run udev_usb monitoring thread: %s",
                  strerror(errno));
        udev_monitor_unref(monitor);
        udev_unref(udev);
        return -4;
    }

    return 0;
}

void *ndm_udev_thread_usb(void *arg) {
    (void)arg;

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(mon_fd, &fds);

        if (select(mon_fd + 1, &fds, NULL, NULL, NULL) > 0) {
            struct udev_device *dev = udev_monitor_receive_device(monitor);
            if (dev) {
                const char *action = udev_device_get_action(dev);
                if (action && strcmp(action, "add") == 0) {
                    const char *devnode = udev_device_get_devnode(dev);
                    if (!devnode) {
                        log_warn("USB device without a device node was added!");
                        udev_device_unref(dev);
                    }

                    const char *devtype = udev_device_get_devtype(dev);
                    const char *fs_type =
                        udev_device_get_property_value(dev, "ID_FS_TYPE");

                    if (!devtype || strcmp(devtype, "partition") != 0) {
                        udev_device_unref(dev);
                        continue;
                    }

                    if (!fs_type) {
                        log_error("Filesystem Type for %s is NULL!", devnode);
                        udev_device_unref(dev);
                    }

                    char buffer[512];
                    snprintf(buffer, sizeof(buffer), "/mnt/usb%d", usb_counter);

                    ndm_mnt_dev(devnode, fs_type, buffer);

                    log_info("Added and mounted USB device %s -> %s", devnode,
                             buffer);

                    usb_counter++;

                } else if (action && strcmp(action, "remove") == 0) {
                    const char *devnode = udev_device_get_devnode(dev);
                    const char *devtype = udev_device_get_devtype(dev);

                    if (!devtype || strcmp(devtype, "partition") != 0) {
                        udev_device_unref(dev);
                        continue;
                    }

                    MountEntry *entry = ndm_mnt_get_entry_devnode(devnode);
                    if (entry) {
                        log_info("Unmounted and removed USB device %s (was "
                                 "mounted at %s)",
                                 devnode, entry->mountpoint);
                        ndm_mnt_unmount(entry->mountpoint);
                        usb_counter--;
                    } else {
                        log_warn(
                            "USB device %s was removed without unmounting!",
                            devnode);
                    }
                }
                udev_device_unref(dev);
            }
        }
    }
    return NULL;
}

int ndm_udev_mount_on_start() {
    if (!udev) {
        udev = udev_new();
        if (!udev) {
            log_error("Failed to retrieve udev!");
            return -1;
        }
    }

    enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        log_error("Failed to retrieve enumerate (object)!");
        return -2;
    }

    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devs = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devs) {
        const char *syspath     = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, syspath);

        if (!dev) {
            log_warn("Device was NULL! Syspath: %s", syspath);
            continue;
        }

        const char *devtype = udev_device_get_devtype(dev);
        if (!devtype || strcmp(devtype, "partition") != 0) {
            udev_device_unref(dev);
            continue;
        }

        const char *devnode = udev_device_get_devnode(dev);
        if (!devnode) {
            log_warn("Could not find device node for device! Syspath: %s",
                     syspath);
            udev_device_unref(dev);
            continue;
        }

        const char *fs_type = udev_device_get_property_value(dev, "ID_FS_TYPE");
        const char *fs_label =
            udev_device_get_property_value(dev, "ID_FS_LABEL");
        const char *fs_usage =
            udev_device_get_property_value(dev, "ID_FS_USAGE");

        if (!fs_usage || strcmp(fs_usage, "filesystem") != 0) {
            log_error("Failed to get device specifics for %s! (fs_type=%s, "
                      "fs_label=%s, fs_usage=%s",
                      devnode, fs_type, fs_label, fs_usage);
            udev_device_unref(dev);
            continue;
        }

        if (!fs_type) {
            log_warn("Filesystem Type for %s is NULL!", devnode);
            fs_type = "";
        }

        if (!fs_label) {
            log_warn("No partition label found for %s!", devnode);
            fs_label = "no-label";
        }

        struct udev_device *parent =
            udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");
        int is_usb = 0;

        if (parent) {
            const char *bus = udev_device_get_property_value(parent, "ID_BUS");
            if (bus && strcmp(bus, "usb") == 0) {
                is_usb = 1;
            }
        }

        char buffer[256];

        if (is_usb == 1) {
            snprintf(buffer, sizeof(buffer), "/mnt/usb%d", usb_counter++);
        } else {
            snprintf(buffer, sizeof(buffer), "/mnt/%d", i++);
        }
        if (strcmp(fs_type, "ntfs") == 0) {
            fs_type = "ntfs-3g";
            char cmd_buf[512];
            log_info("Mounted [NTFS] %s on %s (driver=ntfs-3g)\n", devnode,
                     buffer);
            snprintf(cmd_buf, sizeof(cmd_buf), "sudo ntfs-3g %s %s", devnode,
                     buffer);

            if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
                log_error("Error while creating directory %s: %s", buffer,
                          strerror(errno));
                udev_device_unref(dev);
                continue;
            }

            system(cmd_buf);

            MountEntry *new_entry = malloc(sizeof(MountEntry));
            if (!new_entry) {
                log_error("Error while alloacting memory!");
                return -3; // Memory allocation failed
            }

            new_entry->devnode    = strdup(devnode);
            new_entry->fstype     = strdup(fs_type);
            new_entry->mountpoint = strdup(buffer);
            if (!new_entry->devnode || !new_entry->fstype ||
                !new_entry->mountpoint) {
                log_error("Error while duplicating strings!");
                free(new_entry->devnode);
                free(new_entry->fstype);
                free(new_entry->mountpoint);
                free(new_entry);
                return -4; // Memory allocation failed
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
        } else {
            int res = ndm_mnt_dev(devnode, fs_type, buffer);
            if (res != 0) {
                log_warn("Failed to mount %s -> %s", devnode, buffer);
                udev_device_unref(dev);
                continue;
            }
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    return 0;
}
