#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h> // For close()
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h> // For struct ifreq
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "ble.h"

#define DEVICE_NAME "hci0"

static int open_device(char *device_name) {
    int dev_id = hci_devid(device_name);
    if (dev_id < 0)
        dev_id = hci_get_route(NULL); // Find the device ID of the first available Bluetooth interface

    int dd = hci_open_dev(dev_id);
    if (dd < 0) {
        return -1; // Failed to obtain device descriptor
    }

    return dd;
}

static void send_command(uint8_t ogf, uint16_t ocf, int len, unsigned char *data) {
    struct hci_filter flt;

    int dd = open_device(DEVICE_NAME);

    hci_filter_clear(&flt);
    hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
    hci_filter_all_events(&flt);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        hci_close_dev(dd);
        printf("HCI filter set up failed");
        exit(EXIT_FAILURE);
    }

    if (hci_send_cmd(dd, ogf, ocf, len, data) < 0) {
        hci_close_dev(dd);
        printf("Failed to send command to controller");
        exit(EXIT_FAILURE);
    }

    hci_close_dev(dd);
}

static char* get_ip_addr(const char *interface) {
    struct ifreq ifr;
    static char ip_address[INET_ADDRSTRLEN];

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        return NULL;
    }

    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
        perror("ioctl");
        close(fd);
        return NULL;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip_address, sizeof(ip_address));
    close(fd);
    return ip_address;
}

static void set_advertisement_data(const char *interface) {
    char* ip_address = get_ip_addr(interface);
    int byte1, byte2, byte3, byte4;

    sscanf(ip_address, "%d.%d.%d.%d", &byte1, &byte2, &byte3, &byte4);

    uint8_t advertisement[] = {
        0x0f, 0x02, 0x01, 0x1a, 0x0b, 0xff, 0x4c, 0x00,
        0x09, 0x06, 0x03,
        0x30,
        (uint8_t)byte1, (uint8_t)byte2, (uint8_t)byte3, (uint8_t)byte4
    };

    send_command(0x08, 0x0008, 16, advertisement);
}

void configure_ble(const char *interface, uint8_t *ble_address) {
    uint8_t configure_cmd[] = {
        0xa0, 0x00,
        0xa0, 0x00,
        0x03,
        0x01,
        0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x07,
        0x00
    };

    send_command(0x08, 0x0006, 15, configure_cmd);

    send_command(0x08, 0x0005, 6, ble_address);

    set_advertisement_data(interface);
}

void ble_advertise(bool on) {
    uint8_t cmd[] = {
        on ? 1 : 0
    };
    send_command(0x08, 0x000a, 1, cmd);
}

