#include "finetune_panel.h"
#include "state.h"
#include "spdlog/spdlog.h"
#include "config.h"

#include <algorithm>

LV_IMG_DECLARE(home_z);
LV_IMG_DECLARE(z_closer);
LV_IMG_DECLARE(z_farther);
LV_IMG_DECLARE(pa_plus_img);
LV_IMG_DECLARE(pa_minus_img);
LV_IMG_DECLARE(refresh_img);
LV_IMG_DECLARE(speed_up_img);
LV_IMG_DECLARE(speed_down_img);
LV_IMG_DECLARE(flow_up_img);
LV_IMG_DECLARE(flow_down_img);
LV_IMG_DECLARE(back);

FineTunePanel::FineTunePanel(KWebSocketClient &websocket_client, std::mutex &l)
  : NotifyConsumer(l)
  , ws(websocket_client)
  , panel_cont(lv_obj_create(lv_scr_act()))
  , values_cont(lv_obj_create(panel_cont))
  , zreset_btn(panel_cont, &refresh_img, "Reset Z", &FineTunePanel::_handle_zoffset, this)
  , zup_btn(panel_cont, &z_closer, "Z+", &FineTunePanel::_handle_zoffset, this)
  , zdown_btn(panel_cont, &z_farther, "Z-", &FineTunePanel::_handle_zoffset, this)
  , pareset_btn(panel_cont, &refresh_img, "Reset PA", &FineTunePanel::_handle_pa, this)
  , paup_btn(panel_cont, &pa_plus_img, "PA+", &FineTunePanel::_handle_pa, this)
  , padown_btn(panel_cont, &pa_minus_img, "PA-", &FineTunePanel::_handle_pa, this)
  , speed_reset_btn(panel_cont, &refresh_img, "Speed Reset", &FineTunePanel::_handle_speed, this)    
  , speed_up_btn(panel_cont, &speed_up_img, "Speed+", &FineTunePanel::_handle_speed, this)
  , speed_down_btn(panel_cont, &speed_down_img, "Speed-", &FineTunePanel::_handle_speed, this)
  , flow_reset_btn(panel_cont, &refresh_img, "Flow Reset", &FineTunePanel::_handle_flow, this)
  , flow_up_btn(panel_cont, &flow_up_img, "Flow+", &FineTunePanel::_handle_flow, this)
  , flow_down_btn(panel_cont, &flow_down_img, "Flow-", &FineTunePanel::_handle_flow, this)
  , back_btn(panel_cont, &back, "Back", &FineTunePanel::_handle_callback, this)
  , zoffset_selector(panel_cont, "Z (mm) - PA (mm/s)",
		     {"0.01", "0.05", "0.10", ""}, 0, 30, 15, &FineTunePanel::_handle_callback, this)
  , multipler_selector(panel_cont, "Multipler Step (%)",
		       {"1", "5", "10", "25", ""}, 0, 40, 15, &FineTunePanel::_handle_callback, this)
  , z_offset(values_cont, &home_z, 150, 100, 15, "0.0 mm")
  , pa(values_cont, &pa_plus_img, 150, 100, 15, "0.0 mm/s")
  , speed_factor(values_cont, &speed_up_img, 150, 100, 15 ,"100%")
  , flow_factor(values_cont, &flow_up_img, 150, 100, 15, "100%")
{
  lv_obj_move_background(panel_cont);
  
  lv_obj_set_size(panel_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(panel_cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(values_cont, LV_PCT(20), LV_PCT(80));
  lv_obj_clear_flag(values_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(values_cont, 0, 0);
  lv_obj_set_flex_flow(values_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(values_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  static lv_coord_t grid_main_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

  lv_obj_set_grid_dsc_array(panel_cont, grid_main_col_dsc, grid_main_row_dsc);

  // col 1
  lv_obj_set_grid_cell(zreset_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(zup_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(zdown_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(zoffset_selector.get_container(), LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 3, 1);

  // col 2
  lv_obj_set_grid_cell(pareset_btn.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(paup_btn.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(padown_btn.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  
  // col 3
  lv_obj_set_grid_cell(speed_reset_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(speed_up_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(speed_down_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(multipler_selector.get_container(), LV_GRID_ALIGN_CENTER, 2, 2, LV_GRID_ALIGN_CENTER, 3, 1);  

  // col 4
  lv_obj_set_grid_cell(flow_reset_btn.get_container(), LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(flow_up_btn.get_container(), LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(flow_down_btn.get_container(), LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 2, 1);

  // col 5
  lv_obj_set_grid_cell(values_cont, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 3);  
  lv_obj_set_grid_cell(back_btn.get_container(), LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 3, 1);

  ws.register_notify_update(this);
}

FineTunePanel::~FineTunePanel() {
  if (panel_cont != NULL) {
    lv_obj_del(panel_cont);
    panel_cont = NULL;
  }
  ws.unregister_notify_update(this);
}

void FineTunePanel::foreground() {
  auto v = State::get_instance()->get_data(
		"/printer_state/gcode_move/homing_origin/2"_json_pointer);
  if (!v.is_null()) {
    z_offset.update_label(fmt::format("{:.5} mm", v.template get<double>()).c_str());
  }

  v = State::get_instance()->get_data(
		"/printer_state/extruder/pressure_advance"_json_pointer);
  if (!v.is_null()) {
    pa.update_label(fmt::format("{:.5} mm/s", v.template get<double>()).c_str());
  }

  v = State::get_instance()->get_data(
		"/printer_state/gcode_move/speed_factor"_json_pointer);
  if (!v.is_null()) {
    speed_factor.update_label(fmt::format("{}%",
	   static_cast<int>(v.template get<double>() * 100)).c_str());
  }

  v = State::get_instance()->get_data("/printer_state/gcode_move/extrude_factor"_json_pointer);
  if (!v.is_null()) {
    flow_factor.update_label(fmt::format("{}%",
	   static_cast<int>(v.template get<double>() * 100)).c_str());
  }

  //Set the Z axis buttons
  const bool inverted = Config::get_instance()->get<bool>("/invert_z_icon");
  if (inverted) {
    // UP arrow
    zup_btn.set_image(&z_farther);
    zdown_btn.set_image(&z_closer);
  } else {
    // DOWN arrow
    zup_btn.set_image(&z_closer);
    zdown_btn.set_image(&z_farther);
  }
  
  lv_obj_move_foreground(panel_cont);
}

void FineTunePanel::consume(json &j) {
  std::lock_guard<std::mutex> lock(lv_lock);
  auto v = j["/params/0/gcode_move/homing_origin/2"_json_pointer];
  if (!v.is_null()) {
    z_offset.update_label(fmt::format("{:.5} mm", v.template get<double>()).c_str());
  }

  v = j["/params/0/extruder/pressure_advance"_json_pointer];
  if (!v.is_null()) {
    pa.update_label(fmt::format("{:.5} mm/s", v.template get<double>()).c_str());
  }

  v = j["/params/0/gcode_move/speed_factor"_json_pointer];
  if (!v.is_null()) {
    speed_factor.update_label(fmt::format("{}%",
	   static_cast<int>(v.template get<double>() * 100)).c_str());
  }

  v = j["/params/0/gcode_move/extrude_factor"_json_pointer];
  if (!v.is_null()) {
    flow_factor.update_label(fmt::format("{}%",
	   static_cast<int>(v.template get<double>() * 100)).c_str());
  }
}

void FineTunePanel::handle_callback(lv_event_t *e) {
  spdlog::trace("fine tune btn callback");
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t *selector = lv_event_get_target(e);
    uint32_t idx = lv_btnmatrix_get_selected_btn(selector);

    if (selector == zoffset_selector.get_selector()) {
      zoffset_selector.set_selected_idx(idx);
    }

    if (selector == multipler_selector.get_selector()) {
      multipler_selector.set_selected_idx(idx);
    }
  }
  
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(e);
    if (btn == back_btn.get_container()) {
      lv_obj_move_background(panel_cont);
    }
  }
}

void FineTunePanel::handle_zoffset(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(e);

    if (btn == zreset_btn.get_container()) {
      spdlog::trace("clicked zoffset reset");
      ws.gcode_script("SET_GCODE_OFFSET Z=0 MOVE=1");
    } else {
      const char * step = lv_btnmatrix_get_btn_text(zoffset_selector.get_selector(),
						    zoffset_selector.get_selected_idx());
      spdlog::trace("clicked z {}", step);
      ws.gcode_script(fmt::format("SET_GCODE_OFFSET Z_ADJUST={}{} MOVE=1",
				  btn == zup_btn.get_container() ? "+" : "-",
				  step));
    }
  }
}

void FineTunePanel::handle_pa(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(e);

    if (btn == pareset_btn.get_container()) {
      spdlog::trace("clicked pa reset");
      auto v = State::get_instance()->get_data(
		     "/printer_state/configfile/settings/extruder/pressure_advance"_json_pointer);
      if (!v.is_null()) {
	ws.gcode_script(fmt::format("SET_PRESSURE_ADVANCE ADVANCE={}", v.template get<double>()));
      }
    } else {

      auto cur_pa = State::get_instance()
	->get_data("/printer_state/extruder/pressure_advance"_json_pointer);
      if (!cur_pa.is_null()) {
	const char * step = lv_btnmatrix_get_btn_text(zoffset_selector.get_selector(),
						      zoffset_selector.get_selected_idx());
	
	double direction = btn == paup_btn.get_container() ? std::stod(step) : -std::stod(step);
	double new_pa = cur_pa.template get<double>() + direction;
	new_pa = new_pa < 0 ? 0 : new_pa;
	ws.gcode_script(fmt::format("SET_PRESSURE_ADVANCE ADVANCE={}", new_pa));
      }
    }
  }
}

void FineTunePanel::handle_speed(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(e);
    if (btn == speed_reset_btn.get_container()) {
      spdlog::trace("speed reset");
      ws.gcode_script("M220 S100");
    } else {
      auto spd_factor = State::get_instance()
	->get_data("/printer_state/gcode_move/speed_factor"_json_pointer);
      if (!spd_factor.is_null()) {
	const char * step = lv_btnmatrix_get_btn_text(multipler_selector.get_selector(),
						      multipler_selector.get_selected_idx());

	int32_t direction = btn == speed_up_btn.get_container() ? std::stoi(step) : -std::stoi(step);
	int32_t new_speed = static_cast<int32_t>(spd_factor.template get<double>() * 100 + direction);
	new_speed = std::max(new_speed, 1);
	spdlog::trace("speed step {}, {}", direction, new_speed);
	ws.gcode_script(fmt::format("M220 S{}", new_speed));
      }
    }
  }
}

void FineTunePanel::handle_flow(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(e);
    if (btn == flow_reset_btn.get_container()) {
      spdlog::trace("flow reset");
      ws.gcode_script("M221 S100");
    } else {
      auto extrude_factor = State::get_instance()
	->get_data("/printer_state/gcode_move/extrude_factor"_json_pointer);
      if (!extrude_factor.is_null()) {
	const char * step = lv_btnmatrix_get_btn_text(multipler_selector.get_selector(),
						      multipler_selector.get_selected_idx());

	int32_t direction = btn == flow_up_btn.get_container() ? std::stoi(step) : -std::stoi(step);
	
	int32_t new_flow = static_cast<int32_t>(extrude_factor.template get<double>() * 100 + direction);
	new_flow = std::max(new_flow, 1);
	spdlog::trace("flow step {}, {}", direction, new_flow);
	ws.gcode_script(fmt::format("M221 S{}", new_flow));
      }
    }
  }
}
