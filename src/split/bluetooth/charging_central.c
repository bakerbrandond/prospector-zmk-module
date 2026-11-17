/*
 * Dongle side of the charging indicator: on each keyboard half's connection,
 * find the half's charging characteristic (include/prospector/charging_service.h),
 * subscribe to it, read it once, and raise zmk_peripheral_charging_changed
 * for the battery bar. Halves without the service simply never report.
 *
 * Discovery starts a little after the link is encrypted, so it doesn't
 * compete with ZMK's own split service discovery on the same connection.
 * The subscription is volatile: it's dropped on disconnect and made again on
 * the next connection.
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zmk/event_manager.h>
#include <zmk/events/peripheral_charging_changed.h>
#include <zmk/ble.h>

#include <prospector/charging_service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// ZMK's split central (app/src/split/bluetooth/central.c); not in a header
int peripheral_slot_index_for_conn(struct bt_conn *conn);

#define DISCOVER_DELAY_MS 2000

static const struct bt_uuid_128 char_uuid = BT_UUID_INIT_128(PROSPECTOR_CHARGING_CHAR_UUID);
static const struct bt_uuid_16 ccc_uuid = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);

struct charging_slot {
    struct bt_conn *conn;
    struct k_work_delayable discover_work;
    struct bt_gatt_discover_params char_params;
    struct bt_gatt_discover_params ccc_params;
    struct bt_gatt_subscribe_params sub_params;
    struct bt_gatt_read_params read_params;
    uint16_t value_handle;
};

static struct charging_slot slots[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

static struct charging_slot *slot_for_conn(struct bt_conn *conn) {
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (slots[i].conn == conn) {
            return &slots[i];
        }
    }
    return NULL;
}

static void report(struct bt_conn *conn, const void *data, uint16_t length) {
    int source = peripheral_slot_index_for_conn(conn);
    if (source < 0 || data == NULL || length < 1) {
        return;
    }
    bool charging = ((const uint8_t *)data)[0] != 0;
    LOG_DBG("Peripheral %d %s", source, charging ? "charging" : "not charging");
    raise_zmk_peripheral_charging_changed(
        (struct zmk_peripheral_charging_changed){.source = source, .charging = charging});
}

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length) {
    if (data == NULL) { // unsubscribed
        params->value_handle = 0;
        return BT_GATT_ITER_STOP;
    }
    report(conn, data, length);
    return BT_GATT_ITER_CONTINUE;
}

static uint8_t read_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
                       const void *data, uint16_t length) {
    if (!err) {
        report(conn, data, length);
    }
    return BT_GATT_ITER_STOP;
}

static uint8_t discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           struct bt_gatt_discover_params *params) {
    struct charging_slot *slot = slot_for_conn(conn);
    if (slot == NULL) {
        return BT_GATT_ITER_STOP;
    }
    if (attr == NULL) {
        LOG_DBG("Peripheral has no charging service");
        return BT_GATT_ITER_STOP;
    }

    if (params == &slot->char_params) {
        const struct bt_gatt_chrc *chrc = attr->user_data;
        slot->value_handle = chrc->value_handle;
        slot->ccc_params = (struct bt_gatt_discover_params){
            .uuid = &ccc_uuid.uuid,
            .func = discover_cb,
            .start_handle = chrc->value_handle + 1,
            .end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
            .type = BT_GATT_DISCOVER_DESCRIPTOR,
        };
        int err = bt_gatt_discover(conn, &slot->ccc_params);
        if (err) {
            LOG_ERR("Charging CCC discovery failed: %d", err);
        }
        return BT_GATT_ITER_STOP;
    }

    // CCC found: subscribe, then read the current value once
    slot->sub_params = (struct bt_gatt_subscribe_params){
        .notify = notify_cb,
        .value_handle = slot->value_handle,
        .ccc_handle = attr->handle,
        .value = BT_GATT_CCC_NOTIFY,
    };
    atomic_set_bit(slot->sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
    atomic_set_bit(slot->sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_NO_RESUB);
    int err = bt_gatt_subscribe(conn, &slot->sub_params);
    if (err && err != -EALREADY) {
        LOG_ERR("Charging subscribe failed: %d", err);
    }

    slot->read_params = (struct bt_gatt_read_params){
        .func = read_cb,
        .handle_count = 1,
        .single = {.handle = slot->value_handle, .offset = 0},
    };
    err = bt_gatt_read(conn, &slot->read_params);
    if (err) {
        LOG_ERR("Charging read failed: %d", err);
    }
    return BT_GATT_ITER_STOP;
}

static void discover_work_cb(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct charging_slot *slot = CONTAINER_OF(dwork, struct charging_slot, discover_work);
    if (slot->conn == NULL) {
        return;
    }
    slot->char_params = (struct bt_gatt_discover_params){
        .uuid = &char_uuid.uuid,
        .func = discover_cb,
        .start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
        .type = BT_GATT_DISCOVER_CHARACTERISTIC,
    };
    int err = bt_gatt_discover(slot->conn, &slot->char_params);
    if (err) {
        LOG_ERR("Charging discovery failed: %d", err);
    }
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    if (err || level < BT_SECURITY_L2) {
        return;
    }
    int idx = peripheral_slot_index_for_conn(conn);
    if (idx < 0 || idx >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        return; // a host connection, not a keyboard half
    }
    struct charging_slot *slot = &slots[idx];
    if (slot->conn != NULL && slot->conn != conn) {
        bt_conn_unref(slot->conn);
        slot->conn = NULL;
    }
    if (slot->conn == NULL) {
        slot->conn = bt_conn_ref(conn);
    }
    k_work_reschedule(&slot->discover_work, K_MSEC(DISCOVER_DELAY_MS));
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        struct charging_slot *slot = &slots[i];
        if (slot->conn != conn) {
            continue;
        }
        k_work_cancel_delayable(&slot->discover_work);
        bt_conn_unref(slot->conn);
        slot->conn = NULL;
        slot->value_handle = 0;
        raise_zmk_peripheral_charging_changed(
            (struct zmk_peripheral_charging_changed){.source = i, .charging = false});
    }
}

BT_CONN_CB_DEFINE(prospector_charging_conn_cb) = {
    .security_changed = security_changed,
    .disconnected = disconnected,
};

static int charging_central_init(void) {
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        k_work_init_delayable(&slots[i].discover_work, discover_work_cb);
    }
    return 0;
}

SYS_INIT(charging_central_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
