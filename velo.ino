#include <LowPower.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define WHEEL_PIN 2
#define PEDAL_PIN 3
#define LED_PIN 4
#define BTN_PIN 5
#define LONG_PRESS_TIME 750
#define EEPROM_WHEEL_DIAMETER 0
#define EEPROM_PWR_SAVE_MODE 1

Adafruit_SSD1306 display(128, 32, &Wire, -1);

float wheel_length; // km
uint8_t wheel_pin;
boolean wheel_pin_high;
uint8_t wheel_counter;
float wheel_speed; // km/h
float max_wheel_speed; // km/h
unsigned long wheel_timer; // ms
unsigned long wheel_interval; // ms
uint8_t wheel_interval_counter;
float distance; // km
uint8_t cadence;
uint8_t btn_state;
boolean btn_pressed;
boolean btn_long_pressed;
unsigned long btn_timer; // ms
boolean display_turned;
uint8_t display_menu;
boolean pwr_save_mode;
uint8_t pedal_pin;
boolean pedal_pin_high;
unsigned long pedal_interval; // ms
uint8_t pedal_interval_counter;
unsigned long pedal_timer; // ms
boolean wake_up;
unsigned long timer_now; // ms

void setup() {
    Serial.begin(9600);
    delay(2000);
    
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display();
    delay(2000);

    display_turned = true;
    pwr_save_mode = EEPROM.read(EEPROM_PWR_SAVE_MODE);
    calc_wheel_length(EEPROM.read(EEPROM_WHEEL_DIAMETER));
    display_data();
}

void go_wake_up() {
    wake_up = true;
}

void go_sleep() {
    turn_display(false);
    digitalWrite(LED_PIN, LOW);
    
    attachInterrupt(digitalPinToInterrupt(WHEEL_PIN), go_wake_up, CHANGE);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void calc_wheel_length(int wheel_diameter) {
    if (wheel_diameter == 255) {
        wheel_diameter = 65; // default
        EEPROM.write(EEPROM_WHEEL_DIAMETER, wheel_diameter);
    }
    wheel_diameter *= 3.14;
    wheel_length = wheel_diameter / 100000.0;
}

void switch_display_menu() {
    display_menu++;
    if (display_menu == 5) {
        display_menu = 0;
    }
}

void enable_pwr_save_mode(boolean enable) {
    pwr_save_mode = !pwr_save_mode;
    EEPROM.write(EEPROM_PWR_SAVE_MODE, pwr_save_mode);
}

void display_data() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    switch(display_menu) {
        case 0:
            // speed
            display.setCursor(0, 0);
            display.print(wheel_speed + String(" km/h"));
    
            // cadence
            display.setCursor(0, 16);
            display.print(cadence + String(" rpm"));
            break;
            
        case 1:
            // max speed
            display.setCursor(0, 0);
            display.print(max_wheel_speed + String(" km/h"));
            
            // distance
            display.setCursor(0, 16);
            display.print(distance + String(" km"));
            break;
            
        case 2: // power save mode
            display.setCursor(0, 0);
            display.print("pwr save:");

            display.setCursor(0, 16);
            display.print(pwr_save_mode ? "on" : "off");
            break;

        case 3: // input voltage
            display.setCursor(0, 0);
            display.print("voltage:");

            float v = read_voltage();

            display.setCursor(0, 16);
            display.print(v + String(" v"));
            
            if (v < 3.9) { // charge
                display.print(" !!!");
            }
            break;
    }

    if (display_menu == 4) { // sick!!!
        display.setCursor(0, 0);
        display.print("diameter:");
    
        display.setCursor(0, 16);
        display.print(EEPROM.read(EEPROM_WHEEL_DIAMETER) + String(" cm"));
    }
    
    display.display();
}

void turn_display(boolean on) {
    display_turned = on;
    if (on) {
        display.ssd1306_command(SSD1306_DISPLAYON);
    } else {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
}

float read_voltage() {
    long result;
    // Read 1.1V reference against AVcc
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2); // Wait for Vref to settle
    
    ADCSRA |= _BV(ADSC); // Convert
    while (bit_is_set(ADCSRA, ADSC));
    result = ADCL;
    result |= ADCH << 8;
    result = 1126400L / result; // Back-calculate AVcc in mV
    return result / 1000.0;
}

void loop() {
    if (wake_up) {
        wake_up = false;
        turn_display(true);
        detachInterrupt(digitalPinToInterrupt(WHEEL_PIN));
    }

    timer_now = millis();

    btn_state = digitalRead(BTN_PIN);
    if (!btn_pressed && btn_state == HIGH) {
        btn_pressed = true;
        btn_timer = timer_now;
    }
    if (btn_pressed && btn_state == LOW) {
        btn_pressed = false;
        if (!btn_long_pressed && display_turned) { // single press
            switch_display_menu();
            display_data();
        }
        btn_long_pressed = false;
    }
    if (btn_pressed && !btn_long_pressed && timer_now - btn_timer >= LONG_PRESS_TIME) {
        btn_long_pressed = true;
        if (display_menu == 2) {
            enable_pwr_save_mode(!pwr_save_mode);
            display_data();
            if (pwr_save_mode) {
                wheel_timer = timer_now; // fixme
            }
        } else {
            turn_display(!display_turned); // todo turn on bluetooth
        }
    }

    /*-----------------------------------------------------------------------------*/
    wheel_pin = digitalRead(WHEEL_PIN);
    if (wheel_pin == HIGH && !wheel_pin_high) {
        wheel_pin_high = true;
        if (wheel_timer != 0) {
            wheel_interval += (timer_now - wheel_timer);
            wheel_interval_counter++;
            distance += wheel_length;
        }
        wheel_timer = timer_now;
    } else if (wheel_pin == LOW) {
        wheel_pin_high = false;
    }

    if (wheel_interval >= 3000) {
        float avg = wheel_interval / wheel_interval_counter; // average interval
        avg = 1000 / avg;
        wheel_speed = (avg * 60) * 60 * wheel_length;
        if (wheel_speed >= max_wheel_speed) {
            max_wheel_speed = wheel_speed;
        }
        display_data();
        
        wheel_interval = 0;
        wheel_interval_counter = 0;
    } 
    if (wheel_speed != 0 && timer_now - wheel_timer >= 3000) { // idle
        wheel_speed = 0;
        display_data();
    }
    if (timer_now - wheel_timer >= 30000) { // 30 sec
        wheel_timer = 0;
        if (pwr_save_mode) {
            go_sleep();
        }
    }

    /*-----------------------------------------------------------------------------*/
    pedal_pin = digitalRead(PEDAL_PIN);
    if (pedal_pin == HIGH && !pedal_pin_high) {
        pedal_pin_high = true;
        if (pedal_timer != 0) {
            pedal_interval += (timer_now - pedal_timer);
            pedal_interval_counter++;
        }
        pedal_timer = timer_now;
    } else if (pedal_pin == LOW) {
        pedal_pin_high = false;
    }

    if (pedal_interval >= 3000) {
        float avg = pedal_interval / pedal_interval_counter; // average interval
        avg = 1000 / avg;
        
        cadence = avg * 60;
        digitalWrite(LED_PIN, (cadence >= 70 && cadence <= 100) ? HIGH : LOW);
        display_data();
        
        pedal_interval = 0;
        pedal_interval_counter = 0;
    } else if (pedal_timer != 0 && (timer_now - pedal_timer) >= 3000) { // idle
        pedal_timer = 0;
        
        cadence = 0;
        digitalWrite(LED_PIN, LOW);
        display_data();
    }

    delay(1);
}

//#define CMD_SET_WHEEL_DIAMETER 0
//String bt_cmd; // bluetooth command
//uint8_t bt_cmd_val = -1;
/*
    while (Serial.available()) { // пока приходят данные
        char c = Serial.read(); // считываем их
        if (c == '#') {
            bt_cmd_val = CMD_SET_WHEEL_DIAMETER;
            continue;
        }
        bt_cmd += c; // и формируем строку
        delay(1);
    }

    switch (bt_cmd_val) {
        case CMD_SET_WHEEL_DIAMETER:
            int wheel_diameter = bt_cmd.toInt();
            EEPROM.write(0, wheel_diameter);
            calc_wheel_length(wheel_diameter);

            digitalWrite(LED_PIN, HIGH);
            delay(500);
            digitalWrite(LED_PIN, LOW);
            break;
    }
    bt_cmd_val = -1;
    */

    /*-----------------------------------------------------------------------------*/
