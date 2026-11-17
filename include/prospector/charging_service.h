#pragma once

/*
 * GATT service a keyboard half offers so the Prospector dongle can show when
 * it's charging. ZMK v0.3's split protocol only carries the battery
 * percentage, which reads ~100% while charging because it's derived from the
 * battery voltage.
 *
 * One characteristic, read + notify, encrypted: 1 byte, 1 while the half is
 * on USB power, else 0.
 */

#include <zephyr/bluetooth/uuid.h>

#define PROSPECTOR_CHARGING_SERVICE_UUID                                                           \
    BT_UUID_128_ENCODE(0xe934cf52, 0x73c7, 0x4cd1, 0x856d, 0xe7b61fe9c980)
#define PROSPECTOR_CHARGING_CHAR_UUID                                                              \
    BT_UUID_128_ENCODE(0xe934cf52, 0x73c7, 0x4cd1, 0x856d, 0xe7b61fe9c981)
