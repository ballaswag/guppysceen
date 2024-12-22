#include "belts_calibration_panel.h"
#include "utils.h"
#include "config.h"
#include "spdlog/spdlog.h"

LV_IMG_DECLARE(resume);
LV_IMG_DECLARE(inputshaper_img);
LV_IMG_DECLARE(emergency);
LV_IMG_DECLARE(back);

#define BELTS_PNG "belts_calibration.png"

std::vector<std::string> BeltsCalibrationPanel::axes = {
  "x",
  "y",
  "a",
  "b"
};

BeltsCalibrationPanel::BeltsCalibrationPanel(KWebSocketClient &c, std::mutex &l)
  : ws(c)
  , lv_lock(l)
  , cont(lv_obj_create(lv_scr_act()))
  , graph_cont(lv_obj_create(cont))
  , graph(lv_img_create(graph_cont))
  , spinner(lv_spinner_create(cont, 1000, 60))
  , excite_control(lv_obj_create(cont))
  , excite_slider(lv_slider_create(excite_control))
  , excite_label(lv_label_create(excite_control))
  , excite_dd(lv_dropdown_create(excite_control))
  , button_cont(lv_obj_create(cont))
  , calibrate_btn(button_cont, &resume, "Shake Belts", &BeltsCalibrationPanel::_handle_callback, this)
  , excite_btn(button_cont, &inputshaper_img, "Excitate", &BeltsCalibrationPanel::_handle_callback, this)
  , emergency_btn(button_cont, &emergency, "Stop", &BeltsCalibrationPanel::_handle_callback, this,
		  "Do you want to emergency stop?",
		  [&c]() {
		    spdlog::debug("emergency stop pressed");
		    c.send_jsonrpc("printer.emergency_stop");
		  })
  , back_btn(button_cont, &back, "Back", &BeltsCalibrationPanel::_handle_callback, this)
  , image_fullsized(false)
{
  lv_obj_move_background(cont);

  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(cont, 0, 0);

  lv_obj_set_style_pad_all(graph_cont, 0, 0);
  lv_obj_add_flag(graph_cont, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(graph_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(graph_cont, &BeltsCalibrationPanel::_handle_image_clicked,
		      LV_EVENT_CLICKED, this);
  lv_obj_set_size(graph_cont, LV_PCT(50), LV_PCT(50));

  lv_img_set_zoom(graph, 100);
  lv_obj_center(graph);

  lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(spinner, 100, 100);

  auto scale = (double)lv_disp_get_physical_hor_res(NULL) / 800.0;
  auto hscale = (double)lv_disp_get_physical_ver_res(NULL) / 480.0;

  // excite controls
  lv_obj_t *label = lv_label_create(excite_control);
  lv_label_set_text(label, "Excite Frequency Control");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_clear_flag(excite_control, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_width(excite_control, LV_PCT(80));
  lv_obj_align(excite_slider, LV_ALIGN_LEFT_MID, 75 * scale, 0);
  lv_obj_set_width(excite_slider, LV_PCT(60));
  lv_slider_set_range(excite_slider, 10, 1400);

  lv_obj_add_event_cb(excite_slider, &BeltsCalibrationPanel::_handle_update_slider,
		      LV_EVENT_VALUE_CHANGED, this);
  
  lv_obj_align_to(excite_label, excite_slider, LV_ALIGN_BOTTOM_MID, 0, 35 * hscale);
  lv_label_set_text(excite_label, "1 hz");
  
  lv_dropdown_set_options(excite_dd, fmt::format("{}", fmt::join(axes, "\n")).c_str());
  lv_obj_align(excite_dd, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_obj_set_flex_flow(button_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(button_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_size(button_cont, LV_PCT(100), LV_SIZE_CONTENT);

  static lv_coord_t grid_main_row_dsc[] = {LV_GRID_FR(4), LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  
  lv_obj_set_grid_dsc_array(cont, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_set_grid_cell(graph_cont, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_START, 0, 1);
  lv_obj_set_grid_cell(spinner, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(excite_control, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);

  lv_obj_set_grid_cell(button_cont, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);

  ws.register_method_callback("notify_gcode_response",
			      "BeltsCalibrationPanel",
			      [this](json& d) { this->handle_macro_response(d); });
}

BeltsCalibrationPanel::~BeltsCalibrationPanel() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }
}

void BeltsCalibrationPanel::foreground() {
  lv_obj_move_foreground(cont);
}

void BeltsCalibrationPanel::handle_callback(lv_event_t *event) {
  Config *conf = Config::get_instance();
  const std::string belts_shaper_calibration_macro = conf->get<std::string>("/default_macros/belts_shaper_calibration");
  const std::string excitate_axis_at_frequency_macro = conf->get<std::string>("/default_macros/excitate_axis_at_frequency");

  lv_obj_t *btn = lv_event_get_current_target(event);
  if (btn == calibrate_btn.get_container()) {
    auto config_root = KUtils::get_root_path("config");
    auto png_path = fmt::format("{}/{}", config_root.length() > 0 ? config_root : "/tmp" , BELTS_PNG);

    if (!KUtils::is_homed()) {
      ws.gcode_script("G28");
    }

    auto screen_width = (double)lv_disp_get_physical_hor_res(NULL) / 100.0;
    auto screen_height = (double)lv_disp_get_physical_ver_res(NULL) / 100.0;
    ws.gcode_script(fmt::format("{} PNG_OUT_PATH={} PNG_WIDTH={} PNG_HEIGHT={}",
				belts_shaper_calibration_macro, png_path, screen_width, screen_height));

    lv_obj_add_flag(graph, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(graph, NULL);    
    lv_obj_invalidate(graph);
    lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(spinner);

  } else if (btn == excite_btn.get_container()) {
    double excite_hz = (double)lv_slider_get_value(excite_slider) / 10.0 + 1; // [x-1, x+1]
    char excite_buf[10];
    lv_dropdown_get_selected_str(excite_dd, excite_buf, sizeof(excite_buf));

    if (!KUtils::is_homed()) {
      ws.gcode_script("G28");
    }
    ws.gcode_script(fmt::format("{} FREQUENCY={} AXIS={}",
        excitate_axis_at_frequency_macro, excite_hz, excite_buf));

  } else if (btn == back_btn.get_container()) {
    lv_obj_move_background(cont);
  } else if (btn == emergency_btn.get_container()) {
    ws.send_jsonrpc("printer.emergency_stop");
  }
}

void BeltsCalibrationPanel::handle_image_clicked(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    lv_obj_t *clicked = lv_event_get_target(e);

    if (clicked == graph_cont) {
      if (image_fullsized) {
	lv_img_set_zoom(graph, 100);
	lv_obj_set_size(graph_cont, LV_PCT(50), LV_PCT(50));
	lv_obj_clear_flag(graph_cont, LV_OBJ_FLAG_FLOATING);	
	
      } else {
	lv_img_set_zoom(graph, LV_IMG_ZOOM_NONE);
	lv_obj_set_size(graph_cont, LV_PCT(100), LV_PCT(100));
	lv_obj_add_flag(graph_cont, LV_OBJ_FLAG_FLOATING);
      }
      lv_obj_move_foreground(graph_cont);      
      image_fullsized = !image_fullsized;
    } 
  }
}

void BeltsCalibrationPanel::handle_macro_response(json &j) {
  spdlog::trace("belts calibration macro response: {}", j.dump());
  auto &v = j["/params/0"_json_pointer];
  if (!v.is_null()) {
    std::string resp = v.template get<std::string>();
    std::lock_guard<std::mutex> lock(lv_lock);
    if (resp.rfind("// Command {graph_belts} finished", 0) == 0) {
      spdlog::trace("belts calibration finished");
      auto config_root = KUtils::get_root_path("config");
      auto png_path = fmt::format("{}/{}", config_root.length() > 0 ? config_root : "/tmp" , BELTS_PNG);

      png_path = fmt::format("A:{}", png_path);

      lv_img_set_src(graph, png_path.c_str());
      lv_obj_clear_flag(graph, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_background(spinner);
    }
  }
}

void BeltsCalibrationPanel::handle_update_slider(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    double hz = (double)lv_slider_get_value(excite_slider);
    lv_slider_set_value(excite_slider, hz, LV_ANIM_OFF);
    lv_label_set_text(excite_label, fmt::format("{} hz", hz / 10.0 ).c_str());
  }
}
