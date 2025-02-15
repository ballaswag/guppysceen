#include "extruder_panel.h"
#include "state.h"
#include "config.h"
#include "spdlog/spdlog.h"

#include <limits>

LV_IMG_DECLARE(back);
LV_IMG_DECLARE(spoolman_img);
LV_IMG_DECLARE(extrude_img);
LV_IMG_DECLARE(retract_img);
LV_IMG_DECLARE(unload_filament_img);
LV_IMG_DECLARE(load_filament_img);
LV_IMG_DECLARE(extruder);
LV_IMG_DECLARE(cooldown_img);

ExtruderPanel::ExtruderPanel(KWebSocketClient &websocket_client,
			     std::mutex &lock,
			     Numpad &numpad,
			     SpoolmanPanel &sm)
  : NotifyConsumer(lock)
  , ws(websocket_client)
  , panel_cont(lv_obj_create(lv_scr_act()))
  , spoolman_panel(sm)
  , extruder_temp(ws, panel_cont, &extruder, 150,
	  "Extruder", lv_palette_main(LV_PALETTE_RED), false, true, numpad, "extruder", NULL, NULL)
  , temp_selector(panel_cont, "Extruder Temperature (C)",
		  {"180", "190", "200", "210", "220", "230", "240", "\n", "250", "260", "270", "280", "290", "300", ""},
		  6, 62, 30, &ExtruderPanel::_handle_callback, this)
  , length_selector(panel_cont, "Extrude Length (mm)",
		    {"5", "10", "15", "20", "25", "30", "35", ""},
		    1, 62, 15, &ExtruderPanel::_handle_callback, this)
  , speed_selector(panel_cont, "Extrude Speed (mm/s)",
		   {"1", "2", "5", "10", "25", "35", "50", ""},
		   2, 62, 15, &ExtruderPanel::_handle_callback, this)
  , rightside_btns_cont(lv_obj_create(panel_cont))
  , leftside_btns_cont(lv_obj_create(panel_cont))
  , load_btn(leftside_btns_cont, &load_filament_img, "Load", &ExtruderPanel::_handle_callback, this)
  , unload_btn(leftside_btns_cont, &unload_filament_img, "Unload", &ExtruderPanel::_handle_callback, this)
  , cooldown_btn(leftside_btns_cont, &cooldown_img, "Cooldown", &ExtruderPanel::_handle_callback, this)
  , spoolman_btn(rightside_btns_cont, &spoolman_img, "Spoolman", &ExtruderPanel::_handle_callback, this)
  , extrude_btn(rightside_btns_cont, &extrude_img, "Extrude", &ExtruderPanel::_handle_callback, this)
  , retract_btn(rightside_btns_cont, &retract_img, "Retract", &ExtruderPanel::_handle_callback, this)
  , back_btn(rightside_btns_cont, &back, "Back", &ExtruderPanel::_handle_callback, this)
  , load_filament_macro("LOAD_FILAMENT")
  , unload_filament_macro("UNLOAD_FILAMENT")
  , cooldown_macro("SET_HEATER_TEMPERATURE HEATER=extruder TARGET=0")
{
  Config *conf = Config::get_instance();
  auto df = conf->get_json("/default_printer");
  if (!df.empty()) {
    auto v = conf->get_json(conf->df() + "default_macros/load_filament");
    if (!v.is_null()) {
      load_filament_macro = v.template get<std::string>();
    }

    v = conf->get_json(conf->df() + "default_macros/unload_filament");
    if (!v.is_null()) {
      unload_filament_macro = v.template get<std::string>();
    }

    v = conf->get_json(conf->df() + "default_macros/cooldown");
    if (!v.is_null()) {
      cooldown_macro = v.template get<std::string>();
    }
  }

  lv_obj_move_background(panel_cont);
  lv_obj_clear_flag(panel_cont, LV_OBJ_FLAG_SCROLLABLE);  
  lv_obj_set_size(panel_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(panel_cont, 0, 0);

  lv_obj_set_size(rightside_btns_cont, LV_PCT(20), LV_PCT(100));  
  lv_obj_set_flex_flow(rightside_btns_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(rightside_btns_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(rightside_btns_cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(leftside_btns_cont, LV_PCT(20), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_row(leftside_btns_cont, 15, 0);
  lv_obj_set_flex_flow(leftside_btns_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(leftside_btns_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(leftside_btns_cont, LV_OBJ_FLAG_SCROLLABLE);
  
  spoolman_btn.disable();  

  static lv_coord_t grid_main_row_dsc[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(3), LV_GRID_FR(6),
    LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(2), LV_GRID_FR(7), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
  
  lv_obj_clear_flag(panel_cont, LV_OBJ_FLAG_SCROLLABLE);
  
  lv_obj_set_grid_dsc_array(panel_cont, grid_main_col_dsc, grid_main_row_dsc);
  lv_obj_add_flag(extruder_temp.get_sensor(), LV_OBJ_FLAG_FLOATING);
  lv_obj_align(extruder_temp.get_sensor(), LV_ALIGN_TOP_LEFT, 50, 0);

  // lv_obj_set_size(extruder_temp.get_sensor(), 350, 60);
  // col 0
  // lv_obj_set_grid_cell(spoolman_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_START, 0, 2);
  // lv_obj_set_grid_cell(load_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_END, 0, 2);
  // lv_obj_set_grid_cell(unload_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_START, 2, 2);
  // lv_obj_set_grid_cell(cooldown_btn.get_container(), LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_END, 2, 2);

  lv_obj_set_grid_cell(leftside_btns_cont, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 3);
  
  // col 1
  // lv_obj_set_grid_cell(extruder_temp.get_sensor(), LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(speed_selector.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(length_selector.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(temp_selector.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);
  
  // col 2
  // lv_obj_set_grid_cell(spoolman_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_START, 0, 2);
  // lv_obj_set_grid_cell(retract_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_END, 0, 2);
  // lv_obj_set_grid_cell(extrude_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_START, 2, 2);
  // lv_obj_set_grid_cell(back_btn.get_container(), LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_END, 2, 2);

  lv_obj_set_grid_cell(rightside_btns_cont, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_START, 0, 4);
  // lv_obj_set_grid_cell(retract_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_END, 0, 2);
  // lv_obj_set_grid_cell(extrude_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_START, 2, 2);
  // lv_obj_set_grid_cell(back_btn.get_container(), LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_END, 2, 2);
  

  ws.register_notify_update(this);    
}

ExtruderPanel::~ExtruderPanel() {
  if (panel_cont != NULL) {
    lv_obj_del(panel_cont);
    panel_cont = NULL;
  }
}

void ExtruderPanel::foreground() {
  lv_obj_move_foreground(panel_cont);
}

void ExtruderPanel::enable_spoolman() {
  spoolman_btn.enable();
}

void ExtruderPanel::consume(json& j) {
  std::lock_guard<std::mutex> lock(lv_lock);
  auto target_value = j["/params/0/extruder/target"_json_pointer];
  if (!target_value.is_null()) {
    int target = target_value.template get<int>();
    extruder_temp.update_target(target);
  }
  
  auto temp_value = j["/params/0/extruder/temperature"_json_pointer];
  if (!temp_value.is_null()) {   
    int value = temp_value.template get<int>();
    extruder_temp.update_value(value);
  }
}

void ExtruderPanel::handle_callback(lv_event_t *e) {
  spdlog::trace("handling extruder panel callback");
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t *selector = lv_event_get_target(e);
    uint32_t idx = lv_btnmatrix_get_selected_btn(selector);
    const char * v = lv_btnmatrix_get_btn_text(selector, idx);

    if (selector == temp_selector.get_selector()) {
      temp_selector.set_selected_idx(idx);
    }

    if (selector == length_selector.get_selector()) {
      length_selector.set_selected_idx(idx);
    }

    if (selector == speed_selector.get_selector()) {
      speed_selector.set_selected_idx(idx);
    }

    spdlog::trace("selector {} {} {}, {} {} {}", fmt::ptr(selector), idx, v,
		  fmt::ptr(temp_selector.get_selector()),
		  fmt::ptr(length_selector.get_selector()),
		  fmt::ptr(speed_selector.get_selector()));
    
  } else if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(e);

    if (btn == back_btn.get_container()) {
      lv_obj_move_background(panel_cont);
    }

    if (btn == extrude_btn.get_container()) {
      const char * temp = lv_btnmatrix_get_btn_text(temp_selector.get_selector(),
						   temp_selector.get_selected_idx());
      const char * len = lv_btnmatrix_get_btn_text(length_selector.get_selector(),
						   length_selector.get_selected_idx());
      const char *speed = lv_btnmatrix_get_btn_text(speed_selector.get_selector(),
						    speed_selector.get_selected_idx());
      ws.gcode_script(fmt::format("M109 S{}\nM83\nG1 E{} F{}", temp, len, std::stoi(speed) * 60));
    }

    if (btn == retract_btn.get_container()) {
      const char * temp = lv_btnmatrix_get_btn_text(temp_selector.get_selector(),
						   temp_selector.get_selected_idx());
      const char * len = lv_btnmatrix_get_btn_text(length_selector.get_selector(),
						   length_selector.get_selected_idx());
      const char *speed = lv_btnmatrix_get_btn_text(speed_selector.get_selector(),
						    speed_selector.get_selected_idx());
      ws.gcode_script(fmt::format("M109 S{}\nM83\nG1 E-{} F{}", temp, len, std::stoi(speed) * 60));
    }

    if (btn == unload_btn.get_container()) {
      if (unload_filament_macro == "_GUPPY_QUIT_MATERIAL") {
        const char *temp = lv_btnmatrix_get_btn_text(temp_selector.get_selector(),
                                                     temp_selector.get_selected_idx());
        ws.gcode_script(fmt::format("{} EXTRUDER_TEMP={}", unload_filament_macro, temp));
      } else {
        ws.gcode_script(unload_filament_macro);
      }
    }

    if (btn == load_btn.get_container()) {
      if (load_filament_macro == "_GUPPY_LOAD_MATERIAL") {
        const char *temp = lv_btnmatrix_get_btn_text(temp_selector.get_selector(),
                                                     temp_selector.get_selected_idx());
        const char *len = lv_btnmatrix_get_btn_text(length_selector.get_selector(),
                                                    length_selector.get_selected_idx());
        ws.gcode_script(fmt::format("{} EXTRUDER_TEMP={} EXTRUDE_LEN={}", load_filament_macro, temp, len));
      } else {
        ws.gcode_script(load_filament_macro);
      }
    }

    if (btn == cooldown_btn.get_container()) {
      ws.gcode_script(cooldown_macro);
    }

    if (btn == spoolman_btn.get_container()) {
      spoolman_panel.foreground();
    }
  }
}
