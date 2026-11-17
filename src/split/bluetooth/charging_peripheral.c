/*
 * Keyboard half side of the charging indicator: a GATT service whose single
 * characteristic says whether this half is on USB power, notified to the
 * Prospector dongle whenever that changes. See include/prospector/charging_service.h.
 *
 * "On USB power" stands in for "charging": the nice!nano can't report the
 * charger's state, and a fully charged battery reads 100% anyway.
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

#include <prospector/charging_service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static uint8_t charging;

static ssize_t read_charging(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
                             uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &charging, sizeof(charging));
}

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t value) {}

BT_GATT_SERVICE_DEFINE(
    prospector_charging_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(PROSPECTOR_CHARGING_SERVICE_UUID)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(PROSPECTOR_CHARGING_CHAR_UUID),
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
                           read_charging, NULL, NULL),
    BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT));

static void notify_work_cb(struct k_work *work) {
    uint8_t now = zmk_usb_is_powered() ? 1 : 0;
    if (now == charging) {
        return;
    }
    charging = now;
    LOG_DBG("USB power %s", charging ? "on" : "off");
    // attrs[2] is the characteristic's value; NULL conn notifies every subscriber
    int err = bt_gatt_notify(NULL, &prospector_charging_svc.attrs[2], &charging, sizeof(charging));
    if (err && err != -ENOTCONN) {
        LOG_DBG("Charging notify failed: %d", err);
    }
}

static K_WORK_DEFINE(notify_work, notify_work_cb);

static int usb_listener(const zmk_event_t *eh) {
    k_work_submit(&notify_work);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(prospector_charging_peripheral, usb_listener);
ZMK_SUBSCRIPTION(prospector_charging_peripheral, zmk_usb_conn_state_changed);

static int charging_init(void) {
    charging = zmk_usb_is_powered() ? 1 : 0;
    return 0;
}

SYS_INIT(charging_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
