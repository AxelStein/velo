#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "std_format.h"
#include "speedometer.h"

#define WHEEL_PIN 2
#define BTN_PIN 3
#define LED_PIN 4
#define SINGLE_PRESS_TIME 50
#define LONG_PRESS_TIME 500
#define MENU_MAIN 0
#define MENU_SPEED 1
#define MENU_RPM 2
#define MENU_DISTANCE 3
#define MENU_TIME 4
#define MENU_LED 5
#define EEPROM_WHEEL_DIAMETER 0
#define EEPROM_TOTAL_DISTANCE 1

Speedometer sp;

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

void setup() {
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    sp.init(WHEEL_PIN, calc_wheel_length(), EEPROM_TOTAL_DISTANCE);
    init_display();
}

void init_display() {
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
    sp.detect_rotation();
    if (sp.idle()) {
        display_data();
    }

    handle_btn_click();
    
    uint32_t timer_now = millis();
    uint32_t timer_diff = timer_now - display_timer;

    // update timer every sec
    bool upd_time = display_menu == MENU_TIME && timer_diff >= 1000;

    // update speed every 4 sec
    bool upd_speed = (display_menu == MENU_MAIN || display_menu == MENU_RPM) && sp.get_speed() > 0 && timer_diff >= 4000;
    
    if (upd_time || upd_speed) {
        display_data();
        display_timer = timer_now;
    }

    // toggle led every 250 ms
    if (led_turned && (timer_now - led_timer) >= 250) {
        PORTD ^= _BV(LED_PIN);
        led_timer = timer_now;
    }
}

float calc_wheel_length() {
    uint8_t diameter = EEPROM.read(EEPROM_WHEEL_DIAMETER);  // cm
    if (diameter == 0xFF) {
        diameter = 64;  // default
        set_wheel_diameter(diameter);
    }
    return (diameter * 3.14) / 100000.0;
}

void set_wheel_diameter(uint8_t diameter) {
    if (diameter >= 0 && diameter < 0xFF) {
        EEPROM.write(EEPROM_WHEEL_DIAMETER, diameter);
    }
}

void handle_btn_click() {
    uint8_t pin_state = digitalRead(BTN_PIN);
    uint32_t timer_now = millis();
    
    // click start
    if (!btn_pressed && pin_state == LOW) {
        btn_pressed = true;
        btn_timer = timer_now;
    }
    
    // handle single button click
    if (btn_pressed && pin_state == HIGH) {
        if (!btn_long_pressed && display_turned && ((timer_now - btn_timer) >= SINGLE_PRESS_TIME)) {
            btn_pressed = false;
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
                sp.save_total_distance();
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

void turn_led(boolean on) {
    led_turned = on;
    digitalWrite(LED_PIN, LOW);
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
            std_ftoa(sp.get_speed(), 1, buf);
            display.print(buf);
            display.print(" km/h ");

            // distance
            display.setCursor(0, 16);
            std_ftoa(sp.get_distance(), 3, buf);
            display.print(buf);
            display.print(" km ");
            break;
            
        case MENU_SPEED:
            // max speed
            display.setCursor(0, 0);
            std_ftoa(sp.get_max_speed(), 1, buf);
            display.print(buf);
            display.print(" km/h ");
            
            // avg speed
            display.setCursor(0, 16);
            std_ftoa(sp.get_avg_speed(), 1, buf);
            display.print(buf);
            display.print(" km/h ");
            break;

        case MENU_RPM:
            // rpm
            display.setCursor(0, 0);
            display.print("rpm: ");

            display.setCursor(0, 16);
            std_itoa(sp.get_wheel_rpm(), 0, buf);
            display.print(buf);
            display.print("  ");
            break;

        case MENU_DISTANCE:
            display.setCursor(0, 0);
            display.print("total: ");

            display.setCursor(0, 16);
            std_ftoa(sp.get_total_distance(), 3, buf);
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
            std_itoa(hours, 2, buf);
            display.print(buf);
            display.print(":");

            std_itoa(minutes, 2, buf);
            display.print(buf);
            display.print(":");
            
            std_itoa(seconds, 2, buf);
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
