#include "core/log.h"
#include "core/mnt.h"
#include "core/udev.h"
#include <libudev.h>
#include <signal.h>
#include <stdlib.h>

void sig_hand(int sig) {
    (void)sig;
    ndm_mnt_unmount_all();
    log_info("Unmounted all devices");
    exit(0);
}

int main(void) {

    signal(SIGINT, sig_hand);
    signal(SIGTERM, sig_hand);

    if (ndm_udev_mount_on_start() != 0) {
        log_error("Failed to initialize udev and mount devices");
        return -1;
    }

    if (ndm_udev_init_usb() != 0) {
        log_error("Failed to initialize udev for USB devices");
        return -1;
    }

    for (;;)
        ;
    return 0;
}
