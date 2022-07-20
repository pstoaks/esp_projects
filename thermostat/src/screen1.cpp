#include <Arduino.h>
#include <esp_sntp.h>

#include "screen1.h"

#include <lvgl.h>
#ifdef ESPI
// https://github.com/Bodmer/TFT_eSPI
// This video has a good summary of how to use the library, including the
// SD card.
// https://www.youtube.com/watch?v=rq5yPJbX_uk&ab_channel=XTronical
//
// User Setup comes from; TFT_eSPI/User_Setups/User_Setup.h
#include <TFT_eSPI.h>
#else
#include "lovyan_gfx_setup.h"
#endif


static lv_obj_t *date_time_label;
static lv_obj_t *meter;
static lv_obj_t *temp_label;
static lv_obj_t *tset_label;
static lv_obj_t *battery_label;
static lv_meter_indicator_t *temp_indic;
static lv_meter_indicator_t *tset_indic;
static lv_anim_t a;

static void set_value(void *indic, int32_t v) 
{
  lv_meter_set_indicator_end_value(meter, (lv_meter_indicator_t *)indic, v);
} // set_value()

void setup_screen(void) 
{
  lv_obj_set_style_bg_color(lv_scr_act(), lv_palette_main(LV_PALETTE_INDIGO), 0);

  meter = lv_meter_create(lv_scr_act());
  lv_obj_align(meter, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_size(meter, 220, 220);
  lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);

  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_ticks(meter, scale, 7, 2, 10, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter, scale, 1, 2, 30, lv_color_hex3(0xeee), 15);
  lv_meter_set_scale_range(meter, scale, 40, 100, 270, 135);

  temp_indic = lv_meter_add_arc(meter, scale, 10, lv_palette_main(LV_PALETTE_RED), 0);
  tset_indic = lv_meter_add_arc(meter, scale, 10, lv_palette_main(LV_PALETTE_BLUE), -20);

  lv_anim_init(&a);
  lv_anim_set_exec_cb(&a, set_value);

  static lv_style_t style_red;
  lv_style_init(&style_red);
  lv_style_set_radius(&style_red, 5);
  lv_style_set_bg_opa(&style_red, LV_OPA_COVER);
  lv_style_set_bg_color(&style_red, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_outline_width(&style_red, 2);
  lv_style_set_outline_color(&style_red, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_outline_pad(&style_red, 4);

  static lv_style_t style_blue;
  lv_style_init(&style_blue);
  lv_style_set_radius(&style_blue, 5);
  lv_style_set_bg_opa(&style_blue, LV_OPA_COVER);
  lv_style_set_bg_color(&style_blue, lv_palette_main(LV_PALETTE_BLUE));
  lv_style_set_outline_width(&style_blue, 2);
  lv_style_set_outline_color(&style_blue, lv_palette_main(LV_PALETTE_BLUE));
  lv_style_set_outline_pad(&style_blue, 4);

  static lv_style_t style_center;
  lv_style_init(&style_center);
  lv_style_set_text_align(&style_center, LV_TEXT_ALIGN_CENTER);
  lv_style_set_text_font(&style_center, &lv_font_montserrat_42);

  temp_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(temp_label, 200);
  lv_label_set_text(temp_label, "70");
  lv_obj_add_style(temp_label, &style_center, LV_PART_MAIN);
  lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *blue_obj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(blue_obj, 10, 10);
  lv_obj_add_style(blue_obj, &style_blue, 0);
  lv_obj_align(blue_obj, LV_ALIGN_CENTER, -60, 110);

  tset_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(tset_label, 200);
  lv_label_set_text(tset_label, "Temp Set: 0 °F");
  lv_obj_align_to(tset_label, blue_obj, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  date_time_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(date_time_label, 100);
  lv_obj_set_style_text_align(date_time_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_font(date_time_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(date_time_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(date_time_label, LV_ALIGN_TOP_RIGHT, -10, 0);
  lv_label_set_text(date_time_label, "07/01/22\n20:10");

} // setup_screen()

void update_time_label() 
{
  struct tm time;
   
  if( !getLocalTime(&time) ) 
  {
    Serial.println("Could not obtain time info");
    return;
  }
 
  char date[12] = "";
  char tod[10] = "";

  snprintf(date, sizeof(date), "%02d/%02d/%02d", time.tm_mon, time.tm_mday, time.tm_year+1900-2000);
  snprintf(tod, sizeof(tod), "%02d:%02d", time.tm_hour, time.tm_min, time);
  
  String date_time = String(date) + "\n" + String(tod);

  lv_label_set_text(date_time_label, date_time.c_str());
} // update_time()

void update_temp_tset_display(float temp_fahren, float temp_set, float humid) 
{

  static int prev_temp = 0;
  int curr_temp = static_cast<int>(temp_fahren+0.5);
  if ( curr_temp != prev_temp )
  {
    lv_label_set_text(temp_label, String(curr_temp).c_str());

    lv_anim_set_var(&a, temp_indic);
    lv_anim_set_time(&a, 500);
    lv_anim_set_values(&a, prev_temp, curr_temp);
    lv_anim_start(&a);
    prev_temp = curr_temp;
  }

  static int prev_tset = 0;
  int curr_tset = static_cast<int>(temp_set+0.5);
  if ( curr_tset != prev_tset )
  {
    lv_label_set_text(tset_label, String(curr_tset).c_str());

    lv_anim_set_var(&a, tset_indic);
    lv_anim_set_time(&a, 500);
    lv_anim_set_values(&a, prev_tset, curr_tset);
    lv_anim_start(&a);
    prev_tset = curr_tset;
  }
} // update_temp_tset_display()
