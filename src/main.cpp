#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "lv_tc.h"
#include "lv_tc_screen.h"

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;


#ifdef SIMULATOR
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#include "lv_drivers/sdl/sdl.h"

static int tick_thread(void *data);

#endif // SIMUALTOR

static void hal_init(lv_color_t p, lv_color_t s);

#include "guppyscreen.h"
#include "hv/hlog.h"
#include "config.h"

#include <algorithm>

using namespace hv;

#define DISP_BUF_SIZE (128 * 1024)

int main(void)
{
    // config
    spdlog::debug("current path {}", std::string(fs::canonical("/proc/self/exe").parent_path()));

    Config *conf = Config::get_instance();
    auto config_path = fs::canonical("/proc/self/exe").parent_path() / "guppyconfig.json";
    conf->init(config_path.string(), "/usr/data/printer_data/thumbnails");

    GuppyScreen::init(hal_init);
    GuppyScreen::loop();
    return 0;
}

#ifndef SIMULATOR

static void hal_init(lv_color_t primary, lv_color_t secondary) {
    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];
    static lv_color_t buf2[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, buf2, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;

    uint32_t width;
    uint32_t height;
    uint32_t dpi;
    fbdev_get_sizes(&width, &height, &dpi);
    
    disp_drv.hor_res    = width;
    disp_drv.ver_res    = height;
    disp_drv.sw_rotate = 1;
    disp_drv.rotated = 3; // hardcoded to 3 for k series

    spdlog::debug("resolution {} x {}", width, height);
    lv_disp_t * disp = lv_disp_drv_register(&disp_drv);
    lv_theme_t * th = height <= 480
      ? lv_theme_default_init(NULL, primary, secondary, true, &lv_font_montserrat_12)
      : lv_theme_default_init(NULL, primary, secondary, true, &lv_font_montserrat_20);
    lv_disp_set_theme(disp, th);

    evdev_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1);
    indev_drv_1.read_cb = evdev_read; // no calibration
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv_1);
}

#else // SIMULATOR

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static void hal_init(lv_color_t primary, lv_color_t secondary)
{
  /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
  sdl_init();
  /* Tick init.
   * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about
   * how much time were elapsed Create an SDL thread to do this*/
  SDL_CreateThread(tick_thread, "tick", NULL);

  /*Create a display buffer*/
  static lv_disp_draw_buf_t disp_buf1;
  static lv_color_t buf1_1[MONITOR_HOR_RES * 100];
  static lv_color_t buf1_2[MONITOR_HOR_RES * 100];
  lv_disp_draw_buf_init(&disp_buf1, buf1_1, buf1_2, MONITOR_HOR_RES * 100);

  /*Create a display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv); /*Basic initialization*/
  disp_drv.draw_buf = &disp_buf1;
  disp_drv.flush_cb = sdl_display_flush;
  spdlog::debug("resolution {} x {}", MONITOR_VER_RES, MONITOR_HOR_RES);
  disp_drv.hor_res = MONITOR_HOR_RES;
  disp_drv.ver_res = MONITOR_VER_RES;
  // disp_drv.sw_rotate = 1;
  // disp_drv.rotated = LV_DISP_ROT_270;
  disp_drv.antialiasing = 1;

  lv_disp_t * disp = lv_disp_drv_register(&disp_drv);
  lv_theme_t * th = MONITOR_HOR_RES <= 480
    ? lv_theme_default_init(NULL, primary, secondary, true, &lv_font_montserrat_12)
    : lv_theme_default_init(NULL, primary, secondary, true, &lv_font_montserrat_16);
  lv_disp_set_theme(disp, th);
 
  lv_group_t * g = lv_group_create();
  lv_group_set_default(g);

  /* Add the mouse as input device
   * Use the 'mouse' driver which reads the PC's mouse*/
  // mouse_init();
  static lv_indev_drv_t indev_drv_1;
  lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
  indev_drv_1.type = LV_INDEV_TYPE_POINTER;

  /*This function will be called periodically (by the library) to get the mouse position and state*/
  indev_drv_1.read_cb = sdl_mouse_read;
  lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

  // keyboard_init();
  static lv_indev_drv_t indev_drv_2;
  lv_indev_drv_init(&indev_drv_2); /*Basic initialization*/
  indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
  indev_drv_2.read_cb = sdl_keyboard_read;
  lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv_2);
  lv_indev_set_group(kb_indev, g);
  // mousewheel_init();
  static lv_indev_drv_t indev_drv_3;
  lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
  indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
  indev_drv_3.read_cb = sdl_mousewheel_read;

  lv_indev_t * enc_indev = lv_indev_drv_register(&indev_drv_3);
  lv_indev_set_group(enc_indev, g);

  // /*Set a cursor for the mouse*/
  // LV_IMG_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
  // lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
  // lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
  // lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/
}

/**
 * A task to measure the elapsed time for LVGL
 * @param data unused
 * @return never return
 */
static int tick_thread(void *data) {
  (void)data;

  while(1) {
    SDL_Delay(5);
    lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
  }

  return 0;
}

#endif // SIMULATOR
