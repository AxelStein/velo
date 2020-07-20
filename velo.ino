#include <EEPROM.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>

#include "GyverButton.h"
#include "std_format.h"
#include "speedometer.h"

#define WHEEL_PIN 2
#define BTN_PIN 3
#define LED_PIN 5
#define LONG_PRESS_TIME 500
#define MENU_MAIN 0
#define MENU_SPEED 1
#define MENU_RPM 2
#define MENU_DISTANCE 3
#define MENU_TIME 4
#define MENU_LED 5
#define EEPROM_WHEEL_DIAMETER 0
#define EEPROM_TOTAL_DISTANCE 1
#define I2C_ADDRESS 0x3C

Speedometer sp;

SSD1306AsciiAvrI2c display;
bool display_turned;
uint8_t display_menu;
// uint32_t display_timer;
char buf[11];
volatile bool upd_time;
volatile bool upd_speed;

bool led_turned;
uint32_t led_timer;

GButton btn(BTN_PIN);

void setup() {
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    
    init_timers();
    
    sp.init(WHEEL_PIN, calc_wheel_length(), EEPROM_TOTAL_DISTANCE);
    init_display();
}

void init_display() {
    display.begin(&Adafruit128x32, I2C_ADDRESS, -1);
    display.setFont(Adafruit5x7);
    display.set2X();
    display.clear();
    display_data();
}

void init_timers() {
    cli();
    
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // set ctc mode
    TCCR1A |= _BV(WGM12);
    TCCR1B |= _BV(WGM12);

    OCR1A = 15625;  // update time every sec
    OCR1B = 62500;  // update speed every 4 sec
    
    // set 1024 prescaler
    TCCR1A |= _BV(CS12) | _BV(CS10);
    TCCR1B |= _BV(CS12) | _BV(CS10);

    // enable timer compare interrupt
    TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B);
    TIFR1 |= _BV(OCF1A) | _BV(OCF1B);

    sei();
}

ISR(TIMER1_COMPA_vect) {
    if (display_menu == MENU_TIME) {
        upd_time = true;
    }
}

ISR(TIMER1_COMPB_vect) {
    if (display_menu == MENU_MAIN || display_menu == MENU_RPM) {
        upd_speed = true;
    }
}

void loop() {
    sp.detect_rotation();
    if (sp.idle()) {
        display_data();
    }

    btn.tick();
    if (btn.isClick()) {
        switch_display_menu();
        display_data();
    }
    if (btn.isHolded()) {
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
    
    if (upd_time || (upd_speed && sp.get_speed() > 0)) {
        display_data();
        upd_time = false;
        upd_speed = false;
        // display_timer = timer_now;
    }

    // toggle led every 250 ms
    uint32_t timer_now = millis();
    if (led_turned && (timer_now - led_timer) >= 250) {
        PORTD ^= _BV(LED_PIN);
        led_timer = timer_now;
    }
}

/*
    uint32_t timer_diff = timer_now - display_timer;
    
    // update time every sec
    bool upd_time = display_menu == MENU_TIME && timer_diff >= 1000;

    // update speed every 4 sec
    bool upd_speed = (display_menu == MENU_MAIN || display_menu == MENU_RPM) && sp.get_speed() > 0 && timer_diff >= 4000;
    */

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

void turn_led(boolean on) {
    led_turned = on;
    digitalWrite(LED_PIN, LOW);
}

void turn_display(boolean on) {
    display_turned = on;
    display.ssd1306WriteCmd(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void switch_display_menu() {
    if (++display_menu == 6) display_menu = 0;
    display.clear();
}

void display_data() {
    switch(display_menu) {
        case MENU_MAIN:
            // speed
            display.setCursor(0, 0);
            std_ftoa(sp.get_speed(), 1, buf);
            display.print(buf);
            display.print(" km/h ");

            // distance
            display.setCursor(0, 2);
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
            display.setCursor(0, 2);
            std_ftoa(sp.get_avg_speed(), 1, buf);
            display.print(buf);
            display.print(" km/h ");
            break;

        case MENU_RPM:
            // rpm
            display.setCursor(0, 0);
            display.print("rpm: ");

            display.setCursor(0, 2);
            std_itoa(sp.get_wheel_rpm(), 0, buf);
            display.print(buf);
            display.print("  ");
            break;

        case MENU_DISTANCE:
            display.setCursor(0, 0);
            display.print("total: ");

            display.setCursor(0, 2);
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

            display.setCursor(0, 2);
            display.print(led_turned ? "on " : "off");
            break;
    }
}