#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define WHEEL_PIN 2
#define CADENCE_PIN 3
#define CADENCE_LED_PIN 4

# define HOUR_CONSTANT 0.00027 // 1 sec in hours
# define CALC_TIME 1000 // 1 sec
# define CADENCE_MULT 12# define WHEEL_LENGTH 0.00207 // in km for 26"

# define COM_SET_WHEEL_LENGTH 1# define COM_SET_CALC_TIME 2

LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t pin;
boolean pin_enabled;

uint8_t wheel_rate;
unsigned long timer; // ms
float wheel_length;
float wheel_speed; // km/h
float max_wheel_speed; // km/h

float distance; // km

uint8_t cadence_pin;
boolean cadence_pin_enabled;

uint8_t cadence_rate; // rpm 70-100
unsigned long cadence_timer; // ms

String bt_cmd; // bluetooth command
uint8_t bt_cmd_val = -1;

void setup() {
    // put your setup code here, to run once:
    lcd.init();
    lcd.backlight();
    lcd.noBlink();
    lcd.print("loading");
}

void loop() {
    // put your main code here, to run repeatedly:
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

    pin = digitalRead(WHEEL_PIN);
    if (pin == HIGH && !pin_enabled) {
        pin_enabled = true;
        wheel_rate++;
    } else if (pin == LOW) {
        pin_enabled = false;
    }

    if (millis() - timer >= CALC_TIME) {
        timer = millis();
        float d = wheel_rate * WHEEL_LENGTH;
        distance += d;

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

    cadence_pin = digitalRead(CADENCE_PIN);
    if (cadence_pin == HIGH && !cadence_pin_enabled) {
        cadence_pin_enabled = true;
        cadence_rate++;
    } else if (cadence_pin == LOW) {
        cadence_pin_enabled = false;
    }

    if (millis() - cadence_timer >= CALC_TIME) {
        cadence_timer = millis();

        cadence_rate *= CADENCE_MULT;
        // display cadence per minute
        lcd.setCursor(9, 0);
        lcd.print("        ");
        lcd.setCursor(9, 0);
        lcd.print(cadence_rate);
        lcd.print(" rpm");

        if (cadence_rate >= 70) {
            // enable diod
            digitalWrite(CADENCE_LED_PIN, HIGH);
        } else {
            // disable diod
            digitalWrite(CADENCE_LED_PIN, LOW);
        }

        cadence_rate = 0;
    }

    delay(1);
}