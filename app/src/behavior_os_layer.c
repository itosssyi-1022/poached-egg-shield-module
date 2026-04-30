#include <zephyr/init.h>
#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>

// Profile 5 を Mac 用として扱い、Layer 5 を OS 差分レイヤーとして重ねる。
#define MAC_OS_LAYER 5

static void update_os_layers(uint8_t profile) {
    // Mac 接続時だけ差分レイヤーを有効にし、それ以外は Windows 既定に戻す。
    if (profile == 5) {
        zmk_keymap_layer_activate(MAC_OS_LAYER);
        return;
    }

    zmk_keymap_layer_deactivate(MAC_OS_LAYER);
}

static int os_layer_listener_cb(const zmk_event_t *eh) {
    // BLE のアクティブプロファイル変更イベントだけを拾ってレイヤー状態を更新する。
    const struct zmk_ble_active_profile_changed *event =
        as_zmk_ble_active_profile_changed(eh);

    if (event != NULL) {
        update_os_layers(event->index);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(os_layer_listener, os_layer_listener_cb);
ZMK_SUBSCRIPTION(os_layer_listener, zmk_ble_active_profile_changed);

static int behavior_os_layer_init(void) {
    // 起動直後にも現在の接続先プロファイルに合わせて初期状態をそろえる。
    update_os_layers(zmk_ble_active_profile_index());
    return 0;
}

SYS_INIT(behavior_os_layer_init, APPLICATION, 95);