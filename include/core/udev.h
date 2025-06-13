#ifndef UDEV_H
#define UDEV_H

int ndm_udev_init_usb();
void *ndm_udev_thread_usb(void *arg);

int ndm_udev_mount_on_start();

#endif // UDEV_H
