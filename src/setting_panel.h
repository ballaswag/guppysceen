#ifndef __SETTING_PANEL_H__
#define __SETTING_PANEL_H__

#include "platform.h"
#include "wifi_panel.h"
#include "sysinfo_panel.h"
#include "spoolman_panel.h"
#include "button_container.h"
#include "websocket_client.h"
#include "lvgl/lvgl.h"

#include <mutex>

class SettingPanel {
 public:
  SettingPanel(KWebSocketClient &c, std::mutex &l, lv_obj_t *parent, SpoolmanPanel &sm);
  ~SettingPanel();

  lv_obj_t *get_container();
  void enable_spoolman();

  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    SettingPanel *panel = (SettingPanel*)event->user_data;
    panel->handle_callback(event);
  };

 private:
  KWebSocketClient &ws;
  lv_obj_t *cont;

  WifiPanel wifi_panel;

  SysInfoPanel sysinfo_panel;
  SpoolmanPanel &spoolman_panel;
  ButtonContainer wifi_btn;
  ButtonContainer restart_klipper_btn;
  ButtonContainer restart_firmware_btn;
  ButtonContainer sysinfo_btn;
  ButtonContainer spoolman_btn;
  ButtonContainer guppy_restart_btn;
  ButtonContainer guppy_update_btn;

};

#endif // __SETTING_PANEL_H__

