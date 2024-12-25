#ifndef __PRINTERTUNE_PANEL_H__
#define __PRINTERTUNE_PANEL_H__

#include "finetune_panel.h"
#include "limits_panel.h"
#include "bedmesh_panel.h"
#include "inputshaper_panel.h"
#include "belts_calibration_panel.h"
#include "button_container.h"
#include "lvgl/lvgl.h"

#include <mutex>
class PrinterTunePanel {
 public:
  PrinterTunePanel(KWebSocketClient &c, std::mutex &l, lv_obj_t *parent, FineTunePanel &);
  ~PrinterTunePanel();

  lv_obj_t *get_container();
  BedMeshPanel &get_bedmesh_panel();
  void init(json &j);
  void handle_callback(lv_event_t *event);

  static void _handle_callback(lv_event_t *event) {
    PrinterTunePanel *panel = (PrinterTunePanel*)event->user_data;
    panel->handle_callback(event);
  };

 private:
  lv_obj_t *cont;
  BedMeshPanel bedmesh_panel;
  FineTunePanel &finetune_panel;
  LimitsPanel limits_panel;
  InputShaperPanel inputshaper_panel;
  BeltsCalibrationPanel belts_calibration_panel;
  ButtonContainer bedmesh_btn;
  ButtonContainer finetune_btn;
  ButtonContainer inputshaper_btn;
  ButtonContainer belts_calibration_btn;
  ButtonContainer limits_btn;
};

#endif // __PRINTERTUNE_PANEL_H__
