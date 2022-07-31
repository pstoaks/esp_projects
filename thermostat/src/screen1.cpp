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


static lv_obj_t *outside_temp_label = nullptr;
static lv_obj_t *outside_baro_label = nullptr;
static lv_obj_t *fr_temp_label = nullptr;
static lv_obj_t *time_label = nullptr;
static lv_obj_t *date_label = nullptr;
static lv_obj_t *meter = nullptr;
static lv_obj_t *temp_label = nullptr;
static lv_obj_t *tset_label = nullptr;
static lv_obj_t *battery_label = nullptr;
static lv_meter_indicator_t *temp_indic = nullptr;
static lv_meter_indicator_t *tset_indic = nullptr;
static lv_anim_t a;

static void set_value(void *indic, int32_t v) 
{
  lv_meter_set_indicator_end_value(meter, (lv_meter_indicator_t *)indic, v);
} // set_value()

void setup_screen(void) 
{
  lv_obj_set_style_bg_color(lv_scr_act(), lv_palette_main(LV_PALETTE_INDIGO), 0);

  meter = lv_meter_create(lv_scr_act());
  lv_obj_align(meter, LV_ALIGN_BOTTOM_MID, 0, -7);
  lv_obj_set_size(meter, 225, 225);
  lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);

  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_ticks(meter, scale, 7, 2, 10, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter, scale, 1, 2, 30, lv_color_hex3(0xeee), 10);
  lv_meter_set_scale_range(meter, scale, 40, 100, 270, 135);

  temp_indic = lv_meter_add_arc(meter, scale, 10, lv_palette_main(LV_PALETTE_RED), 0);
  tset_indic = lv_meter_add_arc(meter, scale, 10, lv_palette_main(LV_PALETTE_BLUE), -15);

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

  // Current temperature, center of screen
  temp_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(temp_label, 200);
  lv_label_set_text(temp_label, "");
  lv_obj_add_style(temp_label, &style_center, LV_PART_MAIN);
  lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 0);

  // Temperature setting, bottom middle
  tset_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(tset_label, 200);
  lv_obj_add_style(tset_label, &style_center, LV_PART_MAIN);
  lv_obj_set_style_text_color(tset_label, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
  lv_label_set_text(tset_label, "");
  lv_obj_align(tset_label, LV_ALIGN_BOTTOM_MID, 0, -20);

  ///////////////////////////////////////////////////////////////////////
  // Time fields
  // @TODO: Time and date should be in a parent object to reduce styling.
  time_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(time_label, 100);
  lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_22, LV_PART_MAIN);
  lv_obj_set_style_text_color(time_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 0);
  lv_label_set_text(time_label, "20:10");

  ///////////////////////////////////////////////////////////////////////
  // Date field
  date_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(date_label, 100);
  lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_font(date_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(date_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_align_to(date_label, time_label, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);
  lv_label_set_text(date_label, "July 29");

  ///////////////////////////////////////////////////////////////////////
  // Outside temperature field
  outside_temp_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(outside_temp_label, 100);
  lv_obj_set_style_text_align(outside_temp_label, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(outside_temp_label, &lv_font_montserrat_22, LV_PART_MAIN);
  lv_obj_set_style_text_color(outside_temp_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(outside_temp_label, LV_ALIGN_TOP_LEFT, 10, 0);
  lv_label_set_text(outside_temp_label, "00.0");

  ///////////////////////////////////////////////////////////////////////
  // Outside humidity and barometric pressure field
  outside_baro_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(outside_baro_label, 100);
  lv_obj_set_style_text_align(outside_baro_label, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(outside_baro_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(outside_baro_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_align_to(outside_baro_label, outside_temp_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
  lv_label_set_text(outside_baro_label, "00.0%\n00.0");

  ///////////////////////////////////////////////////////////////////////
  // Family room temperature
  // @TODO: Time and date should be in a parent object to reduce styling.
  fr_temp_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(fr_temp_label, 100);
  lv_obj_set_style_text_align(fr_temp_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_font(fr_temp_label, &lv_font_montserrat_22, LV_PART_MAIN);
  lv_obj_set_style_text_color(fr_temp_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(fr_temp_label, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
  lv_label_set_text(fr_temp_label, "00.0");

} // setup_screen()

void update_time_label() 
{
  struct tm time;
   
  if( !getLocalTime(&time) ) 
  {
    Serial.println("Could not obtain time info");
    return;
  }
 
  char date[8] = "";
  char tod[12] = "";
  static const char months[13][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "ERR" };
  if ( (time.tm_mon < 0) || (time.tm_mon > 11) )
    time.tm_mon = 12;
  else
    time.tm_mon = time.tm_mon; // 0-index into array

  snprintf(date, sizeof(date), "%3.3s %02d", months[time.tm_mon], time.tm_mday);
  snprintf(tod, sizeof(tod), "%2d:%02d %s", time.tm_hour % 12, time.tm_min, (time.tm_hour<12)?"am":"pm");
  
  lv_label_set_text(time_label, tod );
  lv_label_set_text(date_label, date );
} // update_time()

void update_outside_temp(const float temp, const float humid, const float baro) 
{
  char temps[9] = "";
  char baros[14] = "";
  snprintf(temps, sizeof(temps), "%3.1f°F", temp);
  snprintf(baros, sizeof(baros), "%2.1f%%\n%2.1f\"", humid, baro);

  lv_label_set_text(outside_temp_label, temps);
  lv_label_set_text(outside_baro_label, baros);
} // update_outside_temp()

void update_fam_room_temp(const float temp) 
{
  char temps[9] = "";
  snprintf(temps, sizeof(temps), "%3.1f°F", temp);

  lv_label_set_text(fr_temp_label, temps);
} // update_fam_room_temp()

void update_temp_tset_display(float temp_fahren, float temp_set, float humid) 
{

  char tsets[8] = "";
  static int prev_temp = 0;
  // Update when there is any change of 0.1 degree
  int curr_temp = static_cast<int>(temp_fahren*10.0+0.5);
  if ( curr_temp != prev_temp )
  {
    snprintf(tsets, sizeof(tsets), "%2.1f", temp_fahren);
    lv_label_set_text(temp_label, tsets);

    lv_anim_set_var(&a, temp_indic);
    lv_anim_set_time(&a, 500);
    lv_anim_set_values(&a, static_cast<int>(prev_temp/10.0+0.5), static_cast<int>(curr_temp/10.0+0.5));
    lv_anim_start(&a);
    prev_temp = curr_temp;
  }

  static int prev_tset = 0;
  int curr_tset = static_cast<int>(temp_set*10.0+0.5);
  if ( curr_tset != prev_tset )
  {
    snprintf(tsets, sizeof(tsets), "%2.1f", temp_set);
    lv_label_set_text(tset_label, tsets);

    lv_anim_set_var(&a, tset_indic);
    lv_anim_set_time(&a, 500);
    lv_anim_set_values(&a, static_cast<int>(prev_tset/10.0+0.5), static_cast<int>(curr_tset/10.0+0.5));
    lv_anim_start(&a);
    prev_tset = curr_tset;
  }
} // update_temp_tset_display()
