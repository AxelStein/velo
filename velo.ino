#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define WHEEL_PIN 2
#define PEDAL_PIN 3
#define PEDAL_LED_PIN 4
# define WHEEL_LENGTH 0.00207 // in km for 26"

LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t wheel_pin;
boolean wheel_pin_enabled;

uint8_t wheel_counter;
float wheel_speed; // km/h
float max_wheel_speed; // km/h

float distance; // km

uint8_t pedal_pin;
boolean pedal_pin_high;

unsigned long pedal_interval; // ms
uint8_t pedal_interval_counter;

unsigned long pedal_timer; // ms
unsigned long tick_timer; // ms
unsigned long timer_now; // ms

String bt_cmd; // bluetooth command
uint8_t bt_cmd_val = -1;

void setup() {
    lcd.init();
    lcd.backlight();
    lcd.noBlink();
}

void loop() {
    timer_now = millis();

    /*-----------------------------------------------------------------------------*/
    wheel_pin = digitalRead(WHEEL_PIN);
    if (wheel_pin == HIGH && !wheel_pin_enabled) {
        wheel_pin_enabled = true;
        wheel_counter++;
        distance += WHEEL_LENGTH;
    } else if (wheel_pin == LOW) {
        wheel_pin_enabled = false;
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

    /*-----------------------------------------------------------------------------*/
    if (timer_now - tick_timer >= 3000) {
        tick_timer = timer_now;
        int rpm = 0;
        if (pedal_interval_counter > 1) {
            float avg = pedal_interval / pedal_interval_counter; // average interval
            avg = 1000 / avg;
            rpm = avg * 60;

            pedal_interval = 0;
            pedal_interval_counter = 0;
        } else {
            pedal_timer = 0;
        }

        // display rpm
        lcd.setCursor(9, 0);
        lcd.print("        ");
        lcd.setCursor(9, 0);
        lcd.print(rpm);
        lcd.print(" rpm");

        digitalWrite(PEDAL_LED_PIN, (rpm >= 70 && rpm <= 100) ? HIGH : LOW);

        // display distance
        lcd.setCursor(0, 1);
        lcd.print(distance);
        lcd.print(" km");

        // calculate wheel speed km/h
        float d = wheel_counter * WHEEL_LENGTH;
        wheel_counter = 0;
        wheel_speed = d / 0.00083; // 3 secs in hour
        if (wheel_speed >= max_wheel_speed) {
            max_wheel_speed = wheel_speed;
        }
        
        // display wheel speed
        lcd.setCursor(0, 0);
        lcd.print("       ");
        lcd.setCursor(0, 0);
        lcd.print((int) wheel_speed);
        lcd.print(" km/h");
    }

    delay(1);
    
    /*
    while (Serial.available()) { // пока приходят данные
        char c = Serial.read(); // считываем их
        if (c == '#') {
            bt_cmd_val = COM_SET_WHEEL_LENGTH;
            continue;
        } else if (c == '$') {
            bt_cmd_val = COM_SET_CALC_TIME;
            continue;
        }
        bt_cmd += c; // и формируем строку
        delay(1);
    }

    switch (bt_cmd_val) {
    case COM_SET_WHEEL_LENGTH:
        //wheel_length = bt_cmd.toFloat();
        break;
    case COM_SET_CALC_TIME:
        //bt_cmd.toInt();
        break;
    }
    bt_cmd_val = -1;

    wheel_pin = digitalRead(WHEEL_PIN);
    if (wheel_pin == HIGH && !wheel_pin_enabled) {
        wheel_pin_enabled = true;
        wheel_rate++;
        distance += WHEEL_LENGTH;
    } else if (wheel_pin == LOW) {
        wheel_pin_enabled = false;
    }

    if (millis() - timer >= CALC_TIME) {
        timer = millis();
        //float d = wheel_rate * WHEEL_LENGTH;
        //distance += d;

        // display distance
        lcd.setCursor(0, 1);
        lcd.print(distance);
        lcd.print(" km");

        wheel_rate = 0;
        wheel_speed = d / HOUR_CONSTANT;

        // display wheel_speed
        lcd.setCursor(0, 0);
        lcd.print("       ");
        lcd.setCursor(0, 0);
        lcd.print((int) wheel_speed);
        lcd.print(" km/h");

        if (wheel_speed >= max_wheel_speed) {
            max_wheel_speed = wheel_speed;

            // display max_wheel_speed
            //lcd.print("max: ");
            //lcd.print(wheel_speed);
            //lcd.print(" km/h");
        }
    }
    */

    

    /*
    if (millis() - pedal_timer >= CALC_TIME) {
        pedal_timer = millis();

        pedal_rate *= PEDAL_MULT;
        // display cadence per minute
        lcd.setCursor(9, 0);
        lcd.print("        ");
        lcd.setCursor(9, 0);
        lcd.print(pedal_rate);
        lcd.print(" rpm");

        if (pedal_rate >= 70) {
            // enable diod
            digitalWrite(PEDAL_LED_PIN, HIGH);
        } else {
            // disable diod
            digitalWrite(PEDAL_LED_PIN, LOW);
        }

        pedal_rate = 0;
    }
    */
}
