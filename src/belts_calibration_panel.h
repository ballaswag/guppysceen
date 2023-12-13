#ifndef __BELTS_CALIBRATION_PANEL_H__
#define __BELTS_CALIBRATION_PANEL_H__

#include "websocket_client.h"
#include "button_container.h"
#include "lvgl/lvgl.h"

#include <mutex>

class BeltsCalibrationPanel {
 public:
  BeltsCalibrationPanel(KWebSocketClient &c, std::mutex &l);
  ~BeltsCalibrationPanel();

  void foreground();
  void handle_callback(lv_event_t *event);
  void handle_image_clicked(lv_event_t *event);
  void handle_macro_response(json &j);

  static void _handle_callback(lv_event_t *event) {
    BeltsCalibrationPanel *panel = (BeltsCalibrationPanel*)event->user_data;
    panel->handle_callback(event);
  };
  
  static void _handle_image_clicked(lv_event_t *event) {
    BeltsCalibrationPanel *panel = (BeltsCalibrationPanel*)event->user_data;
    panel->handle_image_clicked(event);
  };

 private:
  KWebSocketClient &ws;
  std::mutex &lv_lock;
  lv_obj_t *cont;
  lv_obj_t *graph_cont;
  lv_obj_t *graph;
  lv_obj_t *spinner;
  lv_obj_t *button_cont;
  ButtonContainer calibrate_btn;
  ButtonContainer emergency_btn;
  ButtonContainer back_btn;
  bool image_fullsized;

};

#endif // __BELTS_CALIBRATION_PANEL_H__
