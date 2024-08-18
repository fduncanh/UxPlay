#ifndef BLE_H
#define BLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int configure_ble(const char *interface, uint8_t *ble_address);

int ble_advertise(bool on);

#ifdef __cplusplus
}
#endif

#endif