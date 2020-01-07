#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#define WHEEL_PIN 2
#define PEDAL_PIN 3
#define PEDAL_LED_PIN 4

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial BTSerial(2, 3); // RX | TX

float wheel_length;
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
    BTSerial.begin(9600);
    
    lcd.init();
    lcd.backlight();
    lcd.noBlink();

    calc_wheel_length(EEPROM.read(0));
}

void calc_wheel_length(int wheel_diameter) {
  if (wheel_diameter < 0) {
      BTSerial.println("Error: wheel_diameter can't be < 0");
      return;
  }
  if (wheel_diameter == 255) {
      wheel_diameter = 65; // default
      EEPROM.write(0, wheel_diameter);
    }
    wheel_diameter *= 3.14;
    wheel_length = wheel_diameter / 100000.000000;
}

void loop() {
    /*-----------------------------------------------------------------------------*/
    /*
    while (BTSerial.available()) { // пока приходят данные
        char c = BTSerial.read(); // считываем их
        if (c == '-') {
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
            break;
    }
    bt_cmd_val = -1;
    */

    /*
    if (Serial.available() && (millis() - timer_now >= 1000)) {
      Serial.println("/" + millis());
    }
    */

    /*-----------------------------------------------------------------------------*/
    
    timer_now = millis();

    /*-----------------------------------------------------------------------------*/
    wheel_pin = digitalRead(WHEEL_PIN);
    if (wheel_pin == HIGH && !wheel_pin_enabled) {
        wheel_pin_enabled = true;
        wheel_counter++;
        distance += wheel_length;
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
        float d = wheel_counter * wheel_length;
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

    /*
        if (BTSerial.available()) {
          BTSerial.println("$" + wheel_speed);
          BTSerial.println("%" + max_wheel_speed);
          BTSerial.println("#" + distance);
          BTSerial.println("*" + rpm);
        }
        */
    
    /*-----------------------------------------------------------------------------*/

    delay(1);
}
