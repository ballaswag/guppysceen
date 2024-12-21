#ifndef __GUPPY_SCREEN_H__
#define __GUPPY_SCREEN_H__

#include <mutex>
#include <functional>

#include "lv_tc.h"
#include "lv_tc_screen.h"
#include "lvgl/lvgl.h"

#include "platform.h"
#include "init_panel.h"
#include "main_panel.h"
#include "spoolman_panel.h"
#include "websocket_client.h"

class GuppyScreen {
 private:
  static GuppyScreen *instance;
  static lv_style_t style_container;
  static lv_style_t style_imgbtn_default;
  static lv_style_t style_imgbtn_pressed;
  static lv_style_t style_imgbtn_disabled;
  static lv_theme_t th_new;
  static lv_obj_t *screen_saver;
  static std::mutex lv_lock;
  static KWebSocketClient ws;

  SpoolmanPanel spoolman_panel;
  MainPanel main_panel;
  InitPanel init_panel;

 public:
  GuppyScreen();
  GuppyScreen(GuppyScreen &o) = delete;
  void operator=(const GuppyScreen &) = delete;

  std::mutex &get_lock();

  void connect_ws(const std::string &url);
  static GuppyScreen *get();
  static GuppyScreen *init(std::function<void(lv_color_t, lv_color_t)> hal_init);
  static void loop();
  static void new_theme_apply_cb(lv_theme_t *th, lv_obj_t *obj);
  static void handle_calibrated(lv_event_t *event);
  static void save_calibration_coeff(lv_tc_coeff_t coeff);
  static void refresh_theme();
};

#endif  // __GUPPY_SCREEN_H__
