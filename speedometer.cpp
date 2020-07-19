#include <Arduino.h>
#include <EEPROM.h>
#include "speedometer.h"

#define WHEEL_RPM_MAX 600

void Speedometer::init(uint8_t wheel_pin, float wheel_length, int eeprom_total_distance) {
    WHEEL_PIN = wheel_pin;
    WHEEL_LENGTH = wheel_length;
    EEPROM_TOTAL_DISTANCE = eeprom_total_distance;

    EEPROM.get(EEPROM_TOTAL_DISTANCE, total_distance);
    if (isnan(total_distance)) {
        EEPROM.put(EEPROM_TOTAL_DISTANCE, 0);
        total_distance = 0;
    }
}

float Speedometer::get_speed() {
    return speed;
}

float Speedometer::get_avg_speed() {
    return avg_speed;
}

float Speedometer::get_max_speed() {
    return max_speed;
}

float Speedometer::get_distance() {
    return distance;
}

float Speedometer::get_total_distance() {
    return total_distance;
}

uint16_t Speedometer::get_wheel_rpm() {
    return wheel_rpm;
}

bool Speedometer::idle() {
    if (speed != 0 && millis() - wheel_rotation_last_time >= 4000) {
        speed = 0;
        wheel_rpm = 0;
        wheel_rotation_counter = 0;
        wheel_rotation_start_time = 0;
        return true;
    }
    return false;
}

void Speedometer::detect_rotation() {
    uint32_t timer_now = millis();
    bool wheel_pin_state = digitalRead(WHEEL_PIN);

    // detect rotation start
    if (!wheel_pin_state && !wheel_rotation_started) {
        wheel_rotation_started = true;
    }
    // detect when magnet passes by the hall sensor
    if (wheel_pin_state && wheel_rotation_started) {
        wheel_rotation_started = false;
        
        wheel_rotation_last_time = timer_now;
        if (wheel_rotation_start_time == 0) {
            wheel_rotation_start_time = timer_now;
        }
  
        wheel_rotation_counter++;
        if (wheel_rotation_counter == 5) {
            calc_speed();

            wheel_rotation_counter = 0;
            wheel_rotation_start_time = 0;
        }
        
        distance += WHEEL_LENGTH;
        period_distance += WHEEL_LENGTH;
        
        // save total distance every km
        if (period_distance >= 1.0) {
            save_total_distance();
        }
    }
}

void Speedometer::calc_speed() {
    uint32_t interval = wheel_rotation_last_time - wheel_rotation_start_time;
    uint16_t avg_interval = interval / 4;

    uint16_t rpm = 60000 / avg_interval;
    if (rpm < WHEEL_RPM_MAX) {
        wheel_rpm = rpm;
        speed = wheel_rpm * 60 * WHEEL_LENGTH;
        if (speed >= max_speed) {
            max_speed = speed;
        }

        if (speed != 0) {
            speed_arr[speed_arr_index++] = speed;
            if (speed_arr_index == 4) {
                calc_avg_speed();
                
                speed_arr_index = 0;
            }
        }
    }
}

void Speedometer::calc_avg_speed() {
    float sum = 0;
    for (uint8_t i = 0; i < 4; i++) {
        sum += speed_arr[i];
    }
    
    sum /= 4;
    if (avg_speed == 0) {
        avg_speed = sum;
    } else {
        avg_speed = (avg_speed + sum) / 2;
    }
}

void Speedometer::save_total_distance() {
    total_distance += period_distance;
    EEPROM.put(EEPROM_TOTAL_DISTANCE, total_distance);
    
    period_distance = 0;
}
