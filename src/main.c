#include "core/log.h"
#include "core/mnt.h"
#include "core/udev.h"
#include <core/arg_parser.h>
#include <libudev.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void sig_hand(int sig) {
    (void)sig;
    ndm_mnt_unmount_all();
    log_info("Unmounted all devices");
    exit(0);
}

#define BUILD_DATE __DATE__

#define NDM_VERSION_PREFIX "a"
#define NDM_VERSION_MAJOR  "0"
#define NDM_VERSION_MINOR  "0"
#define NDM_VERSION_PATCH  "1"

int main(int argc, char **argv) {
    ArgParser *arg_parser = args_parser_create();

    if (arg_parser == NULL) {
        log_error("Failed to create argument parser");
        return -1;
    }

    arg_parser_add_option(arg_parser, 'h', "help", false, NULL);
    arg_parser_add_option(arg_parser, 'v', "version", false, NULL);
    arg_parser_add_option(arg_parser, 'c', "config", false, NULL);

    args_parser_parse_args(arg_parser, argc, argv);

    if (arg_parser_has_option_long(arg_parser, "help") ||
        arg_parser_has_option_short(arg_parser, 'h')) {
        printf("Usage: ndm [options]\n");
        printf("Options:\n");
        printf("  -h, --help       Show this help message\n");
        printf("  -v, --version    Show version information\n");
        printf("  -c, --config     Specify configuration file\n");
        args_parser_free(arg_parser);
        return 0;
    }

    if (arg_parser_has_option_long(arg_parser, "version") ||
        arg_parser_has_option_short(arg_parser, 'v')) {
        printf("ndm (Nekos Disk Manager) %s%s.%s.%s %s\n\n", NDM_VERSION_PREFIX,
               NDM_VERSION_MAJOR, NDM_VERSION_MINOR, NDM_VERSION_PATCH,
               BUILD_DATE);
        printf("Copyright 2025 NotNekodev\n");
        printf("Licensed under the Apache License, Version 2.0 (the "
               "\"License\");");
        printf(" you may not use this file except in compliance with the "
               "License.\n");
        printf("You may obtain a copy of the License at\n");
        printf("    http://www.apache.org/licenses/LICENSE-2.0\n");
        printf("Unless required by applicable law or agreed to in writing,\n"
               "software ");
        printf(
            "distributed under the License is distributed on an\n\"AS IS\" ");
        printf("BASIS,");
        printf("WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express "
               "or ");
        printf("implied.\n");
        printf("See the License for the specific language governing "
               "permissions and limitations under the License.\n\n");
        args_parser_free(arg_parser);
        return 0;
    }

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
