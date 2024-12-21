#include "setting_panel.h"
#include "config.h"
#include "spdlog/spdlog.h"
#include "subprocess.hpp"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
namespace sp = subprocess;

LV_IMG_DECLARE(network_img);
LV_IMG_DECLARE(refresh_img);
LV_IMG_DECLARE(spoolman_img);
LV_IMG_DECLARE(update_img);
LV_IMG_DECLARE(sysinfo_img);

LV_IMG_DECLARE(print);

SettingPanel::SettingPanel(KWebSocketClient &c, std::mutex &l, lv_obj_t *parent, SpoolmanPanel &sm)
  : ws(c)
  , cont(lv_obj_create(parent))
  , wifi_panel(l)
  , sysinfo_panel()
  , spoolman_panel(sm)
  , wifi_btn(cont, &network_img, "WIFI", &SettingPanel::_handle_callback, this)
  , restart_klipper_btn(cont, &refresh_img, "Restart Klipper", &SettingPanel::_handle_callback, this)
  , restart_firmware_btn(cont, &refresh_img, "Restart\nFirmware", &SettingPanel::_handle_callback, this)
  , sysinfo_btn(cont, &sysinfo_img, "System", &SettingPanel::_handle_callback, this)
  , spoolman_btn(cont, &spoolman_img, "Spoolman", &SettingPanel::_handle_callback, this)
  , guppy_restart_btn(cont, &refresh_img, "Restart Guppy", &SettingPanel::_handle_callback, this)
  , guppy_update_btn(cont, &update_img, "Update Guppy", &SettingPanel::_handle_callback, this)
{
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));

  spoolman_btn.disable();

  static lv_coord_t grid_main_row_dsc[] = {LV_GRID_FR(2), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
      LV_GRID_TEMPLATE_LAST};

  lv_obj_set_grid_dsc_array(cont, grid_main_col_dsc, grid_main_row_dsc);

  // row 1
  lv_obj_set_grid_cell(wifi_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_START, 1, 1);
  lv_obj_set_grid_cell(restart_klipper_btn.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_START, 1, 1);
  lv_obj_set_grid_cell(restart_firmware_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_START, 1, 1);
  lv_obj_set_grid_cell(sysinfo_btn.get_container(), LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_START, 1, 1);

  // row 2
  lv_obj_set_grid_cell(spoolman_btn.get_container(), LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_START, 2, 1);
  lv_obj_set_grid_cell(guppy_restart_btn.get_container(), LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_START, 2, 1);
  lv_obj_set_grid_cell(guppy_update_btn.get_container(), LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_START, 2, 1);
//  lv_obj_set_grid_cell(printer_select_btn.get_container(), LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_START, 2, 1);
  
}

SettingPanel::~SettingPanel() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }
}

lv_obj_t *SettingPanel::get_container() {
  return cont;
}

void SettingPanel::handle_callback(lv_event_t *event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(event);

    if (btn == wifi_btn.get_container()) {
      spdlog::trace("wifi pressed");
      wifi_panel.foreground();
    } else if (btn == sysinfo_btn.get_container()) {
      spdlog::trace("setting system info pressed");
      sysinfo_panel.foreground();
    } else if (btn == restart_klipper_btn.get_container()) {
      spdlog::trace("setting restart klipper pressed");
      ws.send_jsonrpc("printer.restart");
    } else if (btn == restart_firmware_btn.get_container()) {
      spdlog::trace("setting restart klipper pressed");
      ws.send_jsonrpc("printer.firmware_restart");
    } else if (btn == spoolman_btn.get_container()) {
      spdlog::trace("setting spoolman pressed");
      spoolman_panel.foreground();
    } else if (btn == guppy_restart_btn.get_container()) {
      spdlog::trace("restart guppy pressed");
      Config *conf = Config::get_instance();
      auto init_script = conf->get<std::string>("/guppy_init_script");
      const fs::path script(init_script);
      if (fs::exists(script)) {
        sp::call({script, "restart"});
      } else {
        spdlog::warn("Failed to restart Guppy Screen. Did not find restart script.");
      }
    } else if (btn == guppy_update_btn.get_container()) {
      spdlog::trace("update guppy pressed");
      Config *conf = Config::get_instance();
      auto update_script = conf->get<std::string>("/guppy_update_script");
      const fs::path script(update_script);
      if (fs::exists(script)) {
      	sp::call(script);
      } else {
	      spdlog::warn("Failed to update Guppy Screen. Did not find update script.");
      }
    }
  }
}

void SettingPanel::enable_spoolman() {
  spoolman_btn.enable();
}
