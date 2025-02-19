#include "sysinfo_panel.h"
#include "utils.h"
#include "config.h"
#include "theme.h"
#include "spdlog/spdlog.h"
#include "guppyscreen.h"

#include <algorithm>
#include <iterator>
#include <map>

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

LV_IMG_DECLARE(back);

std::vector<std::string> SysInfoPanel::log_levels = {
  "trace",
  "debug",
  "info"
};

std::vector<std::string> SysInfoPanel::themes = {
  "blue",
  "red",
  "green",
  "purple",
  "pink",
  "yellow"
};

static std::map<int32_t, uint32_t> sleepsec_to_dd_idx = {
  {-1, 0}, // never
  {300, 1}, // 5 min
  {600, 2}, // 10 min
  {1800, 3}, // 30 min
  {3600, 4}, // 1 hour
  {18000, 5} // 5 hour
};

static std::map<std::string, uint32_t> sleep_label_to_sec = {
  {"Never", -1}, // never
  {"5 Minutes", 300}, // 5 min
  {"10 Minutes", 600}, // 10 min
  {"30 Minutes", 1800}, // 30 min
  {"1 Hour", 3600}, // 1 hour
  {"5 Hours", 18000} // 5 hour
};

SysInfoPanel::SysInfoPanel()
  : cont(lv_obj_create(lv_scr_act()))
  , left_cont(lv_obj_create(cont))
  , right_cont(lv_obj_create(cont))
  , network_label(lv_label_create(right_cont))

    // display sleep
  , disp_sleep_cont(lv_obj_create(left_cont))
  , display_sleep_dd(lv_dropdown_create(disp_sleep_cont))

    // log level
  , ll_cont(lv_obj_create(left_cont))
  , loglevel_dd(lv_dropdown_create(ll_cont))
  , loglevel(1)

    // estop prompt
  , estop_toggle_cont(lv_obj_create(left_cont))
  , prompt_estop_toggle(lv_switch_create(estop_toggle_cont))

    // Z axis icons
  , z_icon_toggle_cont(lv_obj_create(left_cont))
  , z_icon_toggle(lv_switch_create(z_icon_toggle_cont))

  // log level
  , theme_cont(lv_obj_create(left_cont))
  , theme_dd(lv_dropdown_create(theme_cont))
  , theme(0)

  , back_btn(cont, &back, "Back", &SysInfoPanel::_handle_callback, this)
{
  lv_obj_move_background(cont);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);

  lv_obj_clear_flag(left_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(left_cont, LV_PCT(50), LV_PCT(100));
  lv_obj_set_flex_flow(left_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(left_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  lv_obj_clear_flag(right_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(right_cont, LV_PCT(50), LV_PCT(100));  
  lv_obj_set_flex_flow(right_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  Config *conf = Config::get_instance();
  lv_obj_t *l = lv_label_create(disp_sleep_cont);
  lv_obj_set_size(disp_sleep_cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(disp_sleep_cont, 0, 0);
  lv_label_set_text(l, "Display Sleep");
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_align(display_sleep_dd, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_dropdown_set_options(display_sleep_dd,
			  "Never\n"
			  "5 Minutes\n"
			  "10 Minutes\n"
			  "30 Minutes\n"
			  "1 Hour\n"
			  "5 Hours");

  const int32_t sleep_sec = conf->get<int32_t>("/display_sleep_sec");
  const auto &el = sleepsec_to_dd_idx.find(sleep_sec);
  if (el != sleepsec_to_dd_idx.end()) {
    lv_dropdown_set_selected(display_sleep_dd, el->second);
  }
  lv_obj_add_event_cb(display_sleep_dd, &SysInfoPanel::_handle_callback,
		      LV_EVENT_VALUE_CHANGED, this);
  
  lv_obj_set_size(ll_cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(ll_cont, 0, 0);
  l = lv_label_create(ll_cont);
  lv_label_set_text(l, "Log Level");
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_align(loglevel_dd, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_dropdown_set_options(loglevel_dd, fmt::format("{}", fmt::join(log_levels, "\n")).c_str());

  json j_null;
  const std::string log_level = conf->get<std::string>("/log_level");
  auto ll_idx = std::find(log_levels.begin(), log_levels.end(), log_level);
  if (ll_idx != std::end(log_levels)) {
    loglevel = std::distance(log_levels.begin(),ll_idx);
    lv_dropdown_set_selected(loglevel_dd, loglevel);
  }

  lv_obj_add_event_cb(loglevel_dd, &SysInfoPanel::_handle_callback,
		      LV_EVENT_VALUE_CHANGED, this);

  lv_obj_set_size(estop_toggle_cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(estop_toggle_cont, 0, 0);

  l = lv_label_create(estop_toggle_cont);
  lv_label_set_text(l, "Prompt Emergency Stop");
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_align(prompt_estop_toggle, LV_ALIGN_RIGHT_MID, 0, 0);

  const bool prompt_emergency_stop = conf->get<bool>("/prompt_emergency_stop");
  if (prompt_emergency_stop) {
    lv_obj_add_state(prompt_estop_toggle, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(prompt_estop_toggle, LV_STATE_CHECKED);
  }

  lv_obj_add_event_cb(prompt_estop_toggle, &SysInfoPanel::_handle_callback,
		      LV_EVENT_VALUE_CHANGED, this);

    /* Z icon selection */
  lv_obj_set_size(z_icon_toggle_cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(z_icon_toggle_cont, 0, 0);

  l = lv_label_create(z_icon_toggle_cont);
  lv_label_set_text(l, "Invert Z Icon");
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_align(z_icon_toggle, LV_ALIGN_RIGHT_MID, 0, 0);

  const bool invert_z_icon = conf->get<bool>("/invert_z_icon");
  if (invert_z_icon) {
    lv_obj_add_state(z_icon_toggle, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(z_icon_toggle, LV_STATE_CHECKED);
  }

  lv_obj_add_event_cb(z_icon_toggle, &SysInfoPanel::_handle_callback, LV_EVENT_VALUE_CHANGED, this);

  // theme dropdown
  lv_obj_set_size(theme_cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(theme_cont, 0, 0);
  l = lv_label_create(theme_cont);
  lv_label_set_text(l, "Theme Color");
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_align(theme_dd, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_dropdown_set_options(theme_dd, fmt::format("{}", fmt::join(themes, "\n")).c_str());

  const std::string theme_id = conf->get<std::string>("/theme");
  auto theme_idx = std::find(themes.begin(), themes.end(), theme_id);
  if (theme_idx != std::end(themes)) {
      theme = std::distance(themes.begin(), theme_idx);
      lv_dropdown_set_selected(theme_dd, theme);
  }

  lv_obj_add_event_cb(theme_dd, &SysInfoPanel::_handle_callback, LV_EVENT_VALUE_CHANGED, this);
  lv_obj_add_flag(back_btn.get_container(), LV_OBJ_FLAG_FLOATING);
  lv_obj_align(back_btn.get_container(), LV_ALIGN_BOTTOM_RIGHT, 0, 0);
}

SysInfoPanel::~SysInfoPanel() {
  if (cont != NULL) {
    lv_obj_del(cont);
    cont = NULL;
  }
}

void SysInfoPanel::foreground() {
  lv_obj_move_foreground(cont);

  auto ifaces = KUtils::get_interfaces();
  std::vector<std::string> network_detail;
  network_detail.push_back("Network");
  for (auto &iface : ifaces) {
    auto ip = KUtils::interface_ip(iface);
    network_detail.push_back(fmt::format("\t{}: {}", iface, ip));
  }
  lv_label_set_text(network_label, fmt::format("{}\n\nSAF GrumpyScreen\n\tBranch: {}\n\tRevision: {}", fmt::join(network_detail, "\n"),
          GUPPYSCREEN_BRANCH, GUPPYSCREEN_VERSION).c_str());
}

void SysInfoPanel::handle_callback(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t *btn = lv_event_get_current_target(e);

    if (btn == back_btn.get_container()) {
      lv_obj_move_background(cont);
    }
  } else if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t *obj = lv_event_get_target(e);
    Config *conf = Config::get_instance();
    if (obj == loglevel_dd) {
      auto idx = lv_dropdown_get_selected(loglevel_dd);
      if (idx != loglevel) {
        if (loglevel < log_levels.size()) {
          loglevel = idx;
          auto ll = spdlog::level::from_str(log_levels[loglevel]);

          spdlog::set_level(ll);
          spdlog::flush_on(ll);
          spdlog::debug("setting log_level to {}", log_levels[loglevel]);
          conf->set<std::string>("/log_level", log_levels[loglevel]);
          conf->save();
        }
      }
    } else if (obj == prompt_estop_toggle) {
      bool should_prompt = lv_obj_has_state(prompt_estop_toggle, LV_STATE_CHECKED);
      conf->set<bool>("/prompt_emergency_stop", should_prompt);
      conf->save();
    } else if (obj == display_sleep_dd) {
      char buf[64];
      lv_dropdown_get_selected_str(display_sleep_dd, buf, sizeof(buf));
      std::string sleep_label = std::string(buf);
      const auto &el = sleep_label_to_sec.find(sleep_label);
      if (el != sleep_label_to_sec.end())
      {
        conf->set<int32_t>("/display_sleep_sec", el->second);
        conf->save();
      }
    } else if (obj == z_icon_toggle) {
      bool inverted = lv_obj_has_state(z_icon_toggle, LV_STATE_CHECKED);
      conf->set<bool>("/invert_z_icon", inverted);
      conf->save();
    } else if (obj == theme_dd) {
      auto idx = lv_dropdown_get_selected(theme_dd);
      if (idx != theme) {
        theme = idx;
        auto selected_theme = themes[theme];
        conf->set<std::string>("/theme", selected_theme);
        conf->save();
        auto theme_config = fs::canonical(conf->get_path()).parent_path() / "themes" / (selected_theme + ".json");
        ThemeConfig::get_instance()->init(theme_config);
        GuppyScreen::refresh_theme();
      }
    }
  }
}
