#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define WHEEL_PIN 2
#define BTN_PIN 3
#define LED_PIN 4
#define SINGLE_PRESS_TIME 50
#define LONG_PRESS_TIME 500
#define WHEEL_RPM_MAX 600
#define MENU_MAIN 0
#define MENU_SPEED 1
#define MENU_RPM 2
#define MENU_DISTANCE 3
#define MENU_TIME 4
#define MENU_LED 5
#define EEPROM_WHEEL_DIAMETER 0
#define EEPROM_TOTAL_DISTANCE 1

Adafruit_SSD1306 display(128, 32, &Wire, -1);
bool display_turned;
uint8_t display_menu;
uint32_t display_timer;
char buf[11];

bool led_turned;
uint32_t led_timer;

bool btn_pressed;
bool btn_long_pressed;
uint32_t btn_timer;

float wheel_length;                    // km
float distance;                        // km
float total_distance;                  // km
float period_distance;                 // km
bool distance_saved;

float speed;                           // km/h
float max_speed;                       // km/h
float avg_speed;                       // km/h
float speed_arr[8];
uint8_t speed_arr_index;

bool wheel_pin_state;
bool wheel_rotation_started;
uint8_t wheel_rotation_counter;
uint32_t wheel_rotation_last_time;
uint32_t wheel_rotation_start_time;
uint16_t wheel_rpm;

uint32_t timer_now;
uint32_t timer_diff;

void setup() {
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    calc_wheel_length();
    EEPROM.get(EEPROM_TOTAL_DISTANCE, total_distance);
    if (isnan(total_distance)) {
        EEPROM.put(EEPROM_TOTAL_DISTANCE, 0);
        total_distance = 0;
    }

    Wire.begin();
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.dim(false);
    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    display_turned = true;
    
    delay(2000);

    display_data();
}

void loop() {
    timer_now = millis();

    wheel_pin_state = digitalRead(WHEEL_PIN);
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
            calc_speed(timer_now);

            wheel_rotation_counter = 0;
            wheel_rotation_start_time = 0;
        }
        
        distance += wheel_length;
        period_distance += wheel_length;
        
        // save total distance every km
        if (period_distance >= 1.0) {
            save_distance();
        }
    }

    handle_btn_click(digitalRead(BTN_PIN), timer_now);

    // toggle led every 250 ms
    if (led_turned && (timer_now - led_timer) >= 250) {
        PORTD ^= _BV(LED_PIN);
        led_timer = timer_now;
    }
    
    // idle
    if (speed != 0 && timer_now - wheel_rotation_last_time >= 4000) {
        speed = 0;
        wheel_rpm = 0;
        wheel_rotation_counter = 0;
        wheel_rotation_start_time = 0;
        
        if (display_menu == MENU_MAIN || display_menu == MENU_RPM) {
            display_data();
        }
    }
    
    timer_diff = timer_now - display_timer;
    bool upd_time = display_menu == MENU_TIME && timer_diff >= 1000;
    bool upd_speed = (display_menu == MENU_MAIN || display_menu == MENU_RPM) && speed > 0 && timer_diff >= 4000;
    if (upd_time || upd_speed) {
        display_data();
        display_timer = timer_now;
    }
}

void calc_wheel_length() {
    int diameter = EEPROM.read(EEPROM_WHEEL_DIAMETER); // cm
    if (diameter == 255) {
        diameter = 65; // default
        set_wheel_diameter(diameter);
    }
    wheel_length = (diameter * 3.14) / 100000.0;
}

void set_wheel_diameter(int diameter) {
    if (diameter > 0 && diameter < 255) {
        EEPROM.write(EEPROM_WHEEL_DIAMETER, diameter);
    }
}

void save_distance() {
    total_distance += period_distance;
    EEPROM.put(EEPROM_TOTAL_DISTANCE, total_distance);
    
    period_distance = 0;
}

void turn_display(boolean on) {
    display_turned = on;
    display.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void switch_display_menu() {
    display_menu++;
    if (display_menu == 6) {
        display_menu = 0;
    }
}

void display_data() {
    display.clearDisplay();
    
    switch(display_menu) {
        case MENU_MAIN:
            // speed
            display.setCursor(0, 0);
            ftoa(speed, 1, buf);
            display.print(buf);
            display.print(" km/h ");

            // distance
            display.setCursor(0, 16);
            ftoa(distance, 3, buf);
            display.print(buf);
            display.print(" km ");
            break;
            
        case MENU_SPEED:
            // max speed
            display.setCursor(0, 0);
            ftoa(max_speed, 1, buf);
            display.print(buf);
            display.print(" km/h ");
            
            // avg speed
            display.setCursor(0, 16);
            ftoa(avg_speed, 1, buf);
            display.print(buf);
            display.print(" km/h ");
            break;

        case MENU_RPM:
            // rpm
            display.setCursor(0, 0);
            display.print("rpm: ");

            display.setCursor(0, 16);
            itoa(wheel_rpm, 0, buf);
            display.print(buf);
            display.print("  ");
            break;

        case MENU_DISTANCE:
            display.setCursor(0, 0);
            display.print("total: ");
            if (distance_saved) {
                display.print("*");
                distance_saved = false;
            }

            display.setCursor(0, 16);
            ftoa(total_distance, 3, buf);
            display.print(buf);
            display.print(" km");
            break;

        case MENU_TIME: {
            uint32_t now = millis();
            uint32_t sec = now / 1000ul;
            int hours = (sec / 3600ul);
            int minutes = (sec % 3600ul) / 60ul;
            int seconds = (sec % 3600ul) % 60ul;
            
            display.setCursor(0, 0);
            display.print("time:");

            display.setCursor(0, 16);
            itoa(hours, 2, buf);
            display.print(buf);
            display.print(":");

            itoa(minutes, 2, buf);
            display.print(buf);
            display.print(":");
            
            itoa(seconds, 2, buf);
            display.print(buf);
            break;
        }

        case MENU_LED:
            display.setCursor(0, 0);
            display.print("led:");

            display.setCursor(0, 16);
            display.print(led_turned ? "on" : "off");
            break;
    }

    display.display();
}

void turn_led(boolean on) {
    led_turned = on;
    digitalWrite(LED_PIN, LOW);
}

void handle_btn_click(uint8_t pin_state, uint32_t timer_now) {
    // click start
    if (!btn_pressed && pin_state == LOW) {
        btn_pressed = true;
        btn_timer = timer_now;
    }
    
    // handle single button click
    if (btn_pressed && pin_state == HIGH) {
        btn_pressed = false;
        
        if (!btn_long_pressed && display_turned && ((timer_now - btn_timer) >= SINGLE_PRESS_TIME)) {
            switch_display_menu();
            display_data();
        }
        btn_long_pressed = false;
    }
    
    // handle long button click
    if (btn_pressed && !btn_long_pressed && ((timer_now - btn_timer) >= LONG_PRESS_TIME)) {
        btn_long_pressed = true;
        switch (display_menu) {
            case MENU_DISTANCE:
                save_distance();
                
                distance_saved = true;
                display_data();
                break;
            case MENU_LED:
                turn_led(!led_turned);
                display_data();
                break;
            default:
                turn_display(!display_turned);
                break;
        }
    }
}

void calc_speed(uint32_t timer_now) {
    uint32_t interval = timer_now - wheel_rotation_start_time;
    uint16_t avg_interval = interval / 4;

    uint16_t rpm = 60000 / avg_interval;
    if (rpm < WHEEL_RPM_MAX) {
        wheel_rpm = rpm;
        speed = wheel_rpm * 60 * wheel_length;
        if (speed >= max_speed) {
            max_speed = speed;
        }
        calc_avg_speed(speed);
    }
}

void calc_avg_speed(float speed) {
    if (speed == 0) {
        return;
    }
    
    speed_arr[speed_arr_index++] = speed;
    
    if (speed_arr_index == 8) {
        speed_arr_index = 0;
        
        float sum = 0;
        for (uint8_t i = 0; i < 5; i++) {
            sum += speed_arr[i];
        }
        
        sum /= 8;
        if (avg_speed == 0) {
            avg_speed = sum;
        } else {
            avg_speed = (avg_speed + sum) / 2;
        }
    }
}

void itoa(int n, int precision, char *buf) {
    if (n < 0) {
        *buf++ = '-';
        n = -n;
    }
  
    int i = 0, j = 0;
    do {
        buf[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    while (i < precision) buf[i++] = '0';
    buf[i] = '\0';
  
    // reverse
    char tmp;
    for (j = i-1, i = 0; i < j; i++, j--) {
        tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
    }
}

void ftoa(float f, int precision, char *buf) {
    if (f < 0) {
        *buf++ = '-';
        f = -f;
    }

    itoa((int) f, 0, buf);  // convert integer part to string
  
    while (*buf != '\0') buf++;
    *buf++ = '.';

    float d = fmod(f, 1);  // extract floating part
    if (precision < 1) {
        d = 0;
    } else {
        d *= pow(10, precision);  // convert floating part to string
    
        // round floating part
        float dx = fmod(d, 1);
        if (dx < 0.5) {
          d = floor(d);
        } else {
          d = ceilf(d);
        }
    }

    itoa((int) d, precision, buf);  // convert floating part to string
}
