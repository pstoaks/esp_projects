#pragma once

void setup_screen(void);

void update_time_label();

void update_outside_temp(const float temp, const float humid, const float baro);

void update_fam_room_temp(const float temp);

void update_temp_tset_display(float temp_fahren, float temp_set, float humid);

