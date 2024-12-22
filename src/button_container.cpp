#include "button_container.h"
#include "config.h"
#include "spdlog/spdlog.h"

ButtonContainer::ButtonContainer(lv_obj_t *parent,
				 const void *btn_img,
				 const char *text,
				 lv_event_cb_t cb,
				 void* user_data,
				 const std::string &prompt,
				 const std::function<void()> &pcb,
				 const bool force)
  : btn_cont(lv_obj_create(parent))
  , btn(lv_imgbtn_create(btn_cont))
  , label(lv_label_create(btn_cont))
  , prompt_text(prompt)
  , prompt_callback(pcb)
  , force_prompt(force)
{
  lv_obj_set_style_pad_all(btn_cont, 0, 0);
  auto width_scale = (double)lv_disp_get_physical_hor_res(NULL) / 800.0;
  lv_obj_set_size(btn_cont, 150 * width_scale, LV_SIZE_CONTENT);

  lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_add_style(btn_cont, &style, LV_PART_MAIN);
  lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, btn_img, NULL);
  lv_obj_set_width(btn, LV_SIZE_CONTENT);
  // lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
  // lv_obj_add_flag(btn_cont, LV_OBJ_FLAG_EVENT_BUBBLE);

  Config *conf = Config::get_instance();
  const bool prompt_emergency_stop = conf->get<bool>("/prompt_emergency_stop");
  auto prompt_estop = force || prompt_emergency_stop;

  if (cb != NULL && !pcb) {
    lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_RELEASED, this);
    
    lv_obj_add_event_cb(btn_cont, cb, LV_EVENT_CLICKED, user_data);
  } else if (cb != NULL && pcb) {
    if (prompt_estop) {
      lv_obj_add_event_cb(btn_cont, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_CLICKED) {
          ((ButtonContainer*)e->user_data)->handle_prompt();
        }
      }, LV_EVENT_CLICKED, this);
    } else {
      lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_PRESSED, this);
      lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_RELEASED, this);

      lv_obj_add_event_cb(btn_cont, cb, LV_EVENT_CLICKED, user_data);
    }
  }

  lv_label_set_text(label, text);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_palette_darken(LV_PALETTE_GREY, 1), LV_STATE_DISABLED);

  lv_obj_align_to(label, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  // lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 5);
  // lv_obj_set_style_border_width(btn_cont, 2, 0);
}

ButtonContainer::~ButtonContainer() {
}

lv_obj_t *ButtonContainer::get_container() {
  return btn_cont;
}

lv_obj_t *ButtonContainer::get_button() {
  return btn;
}

void ButtonContainer::disable() {
  lv_obj_add_state(btn, LV_STATE_DISABLED);
  lv_obj_add_state(btn_cont, LV_STATE_DISABLED);
  lv_obj_add_state(label, LV_STATE_DISABLED);
  
}

void ButtonContainer::enable() {
  lv_obj_clear_state(btn, LV_STATE_DISABLED);
  lv_obj_clear_state(btn_cont, LV_STATE_DISABLED);
  lv_obj_clear_state(label, LV_STATE_DISABLED);
}

void ButtonContainer::hide() {
  lv_obj_add_flag(btn_cont, LV_OBJ_FLAG_HIDDEN);
}

void ButtonContainer::set_image(const void *img) {
  lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, img, NULL);
}

void ButtonContainer::handle_callback(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    lv_imgbtn_set_state(btn, LV_IMGBTN_STATE_PRESSED);
  } else if (code == LV_EVENT_RELEASED) {
    lv_imgbtn_set_state(btn, LV_IMGBTN_STATE_RELEASED);
  }
}

void ButtonContainer::handle_prompt() {
  static const char *force_btns[] = {"Cancel", "Confirm", ""};
  static const char *btns[] = {"Confirm", "Cancel", ""};

  lv_obj_t *mbox1 = lv_msgbox_create(NULL, NULL, prompt_text.c_str(), (force_prompt ? force_btns : btns), false);
  lv_obj_t *msg = ((lv_msgbox_t*)mbox1)->text;
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);  
  lv_obj_set_width(msg, LV_PCT(100));
  lv_obj_center(msg);
  
  lv_obj_t *btnm = lv_msgbox_get_btns(mbox1);
  lv_btnmatrix_set_btn_ctrl(btnm, 0, LV_BTNMATRIX_CTRL_CHECKED);
  lv_btnmatrix_set_btn_ctrl(btnm, 1, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_add_flag(btnm, LV_OBJ_FLAG_FLOATING);
  lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, 0);
  
  auto hscale = (double)lv_disp_get_physical_ver_res(NULL) / 480.0;

  lv_obj_set_size(btnm, LV_PCT(90), 50 *hscale);
  lv_obj_set_size(mbox1, LV_PCT(50), LV_PCT(35));

  // this is nasty as fuck, I can't figure out how to pass state into the lambda
  if (force_prompt) {
    lv_obj_add_event_cb(mbox1, [](lv_event_t *e) {
      lv_obj_t *obj = lv_obj_get_parent(lv_event_get_target(e));
      uint32_t clicked_btn = lv_msgbox_get_active_btn(obj);
      if(clicked_btn == 1) {
        ((ButtonContainer*)e->user_data)->run_callback();
      }
      lv_msgbox_close(obj);
    }, LV_EVENT_VALUE_CHANGED, this);
  } else {
    lv_obj_add_event_cb(mbox1, [](lv_event_t *e) {
        lv_obj_t *obj = lv_obj_get_parent(lv_event_get_target(e));
        uint32_t clicked_btn = lv_msgbox_get_active_btn(obj);
        if(clicked_btn == 0) {
          ((ButtonContainer*)e->user_data)->run_callback();
        }
        lv_msgbox_close(obj);
      }, LV_EVENT_VALUE_CHANGED, this);
  }

  lv_obj_center(mbox1);
}

void ButtonContainer::run_callback() {
  prompt_callback();
}
