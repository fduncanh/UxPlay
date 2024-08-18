#ifndef BLE_H
#define BLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void configure_ble(const char *interface);

void ble_advertise(bool on);

#ifdef __cplusplus
}
#endif

#endif