#include <LowPower.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BUF_SIZE 11
#define WHEEL_PIN 2
#define PEDAL_PIN 3
#define BTN_PIN 4
#define TEMP_PIN A1
#define LED_PIN 5
#define LONG_PRESS_TIME 750
#define EEPROM_WHEEL_DIAMETER 0
#define EEPROM_PWR_SAVE_MODE 1
#define EEPROM_LED_AUTO 2
#define WHEEL_ROTATION_MAX 5
#define WHEEL_RPM_MAX 600
#define MENU_MAIN 0
#define MENU_SPEED 1
#define MENU_RPM 2
#define MENU_POWER 3
#define MENU_VOLTAGE 4
#define MENU_TEMP 5
#define MENU_LED 6

Adafruit_SSD1306 display(128, 32, &Wire, -1);
boolean display_turned;
boolean pwr_save_mode;

String serial_input = "";
char buf[BUF_SIZE];
char str_tmp[6];

uint8_t display_menu;
boolean sleep_mode;
boolean led_turned;
boolean led_auto;

boolean btn_pressed;
boolean btn_long_pressed;
unsigned long btn_timer; // ms

float wheel_length; // km
float distance; // km

volatile boolean wheel_rotated;
boolean wheel_pin_enabled;
uint16_t wheel_rotation_counter;
unsigned long wheel_rotation_last_time; // ms
unsigned long wheel_rotation_start_time; // ms
uint16_t wheel_rpm;
float speed; // km/h
float max_speed; // km/h
float avg_speed; // km/h
float speed_arr[5];
uint8_t speed_arr_index;

void setup() {
    attachInterrupt(digitalPinToInterrupt(WHEEL_PIN), detect_rotation, RISING);
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    
    Serial.begin(9600);
    Wire.begin();
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.dim(false);
    display.setTextWrap(false);
    display.display();
    delay(2000);

    display_turned = true;
    pwr_save_mode = EEPROM.read(EEPROM_PWR_SAVE_MODE);
    led_auto = EEPROM.read(EEPROM_LED_AUTO);
    if (led_auto) {
        // get current time
        // turn led
    }
    calc_wheel_length();
    display_data();
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

void turn_display(boolean on) {
    display_turned = on;
    if (on) {
        display.ssd1306_command(SSD1306_DISPLAYON);
    } else {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }
}

void switch_display_menu() {
    display_menu++;
    if (display_menu == 5) {
        display_menu = 0;
    }
}

void display_data() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    switch(display_menu) {
        case MENU_MAIN:
            // speed
            display.setCursor(0, 0);
            dtostrf(speed, 4, 1, str_tmp);
            snprintf(buf, BUF_SIZE, "%s km/h", str_tmp);
            display.print(buf);

            // distance
            display.setCursor(0, 16);
            dtostrf(distance, 4, 2, str_tmp);
            snprintf(buf, BUF_SIZE, "%s km", str_tmp);
            display.print(buf);
            break;
            
        case MENU_SPEED:
            // max speed
            display.setCursor(0, 0);
            dtostrf(max_speed, 4, 1, str_tmp);
            snprintf(buf, BUF_SIZE, "%s km/h", str_tmp);
            display.print(buf);
            
            // avg speed
            display.setCursor(0, 16);
            dtostrf(avg_speed, 4, 1, str_tmp);
            snprintf(buf, BUF_SIZE, "%s km/h", str_tmp);
            display.print(buf);
            break;

        case MENU_RPM:
            // rpm
            display.setCursor(0, 0);
            snprintf(buf, BUF_SIZE, "%d rpm", wheel_rpm);
            display.print(buf);
            break;
            
        case MENU_POWER: // power save mode
            display.setCursor(0, 0);
            display.print("pwr save:");

            display.setCursor(0, 16);
            display.print(pwr_save_mode ? "on" : "off");
            break;

        case MENU_VOLTAGE: // input voltage
            display.setCursor(0, 0);
            display.print("voltage:");

            dtostrf(read_voltage(), 4, 1, str_tmp);
            snprintf(buf, BUF_SIZE, "%s v", str_tmp);

            display.setCursor(0, 16);
            display.print(buf);
            break;

        case MENU_TEMP:
            display.setCursor(0, 0);
            display.print("temp:");

            snprintf(buf, BUF_SIZE, "%d v", read_temp());
            display.print(buf);
            break

        case MENU_LED:
            display.setCursor(0, 0);
            display.print("led:");

            display.setCursor(0, 16);
            display.print(led_turned ? "on" : "off");
            break
    }

    display.display();
}

uint8_t read_temp() {
    int reading = analogRead(TEMP_PIN);

    // преобразовываем полученные данные в напряжение. Если используем Arduino 3.3 В, то меняем константу на 3.3
    float voltage = reading * 5.0;
    voltage /= 1024.0;

    //конвертируем 10 мВ на градус с учетом отступа 500 мВ
    return (voltage - 0.5) * 100 ;
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

void enable_pwr_save_mode(boolean enable) {
    pwr_save_mode = !pwr_save_mode;
    EEPROM.write(EEPROM_PWR_SAVE_MODE, pwr_save_mode);
}

void enable_sleep_mode() {
    turn_display(false);
    turn_led(false);
    sleep_mode = true;
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void calc_avg_speed(float speed) {
    if (speed == 0) {
        return;
    }
    
    speed_arr[speed_arr_index++] = speed;
    
    if (speed_arr_index == 5) {
        speed_arr_index = 0;
        float sum = 0;
        for (uint8_t i = 0; i < 5; i++) {
            sum += speed_arr[i];
        }
        
        sum /= 5;
        if (avg_speed == 0) {
            avg_speed = sum;
        } else {
            avg_speed = (avg_speed + sum) / 2;
        }
    }
}

void turn_led(boolean on) {
    led_turned = on;

    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    if (on) {
        // disable interrupts
        cli();

        // set ctc mode
        TCCR1B |= (1<<WGM12);

        // set counter max value for 250 ms
        OCR1A = 3906;
        
        // set 1024 prescaler
        TCCR1B |= (1<<CS12) | (1<<CS10);

        // enable timer compare interrupt
        TIMSK1 |= (1<<OCIE1A);

        // allow interrupts
        sei();
    }
}

void handle_btn_click(uint8_t pin_state, unsigned long timer_now) {
    // click start
    if (!btn_pressed && pin_state == LOW) {
        btn_pressed = true;
        btn_timer = timer_now;
    }
    
    // handle single button click
    if (btn_pressed && pin_state == HIGH) {
        btn_pressed = false;
        if (!btn_long_pressed && display_turned) { // single press
            switch_display_menu();
            display_data();
        }
        btn_long_pressed = false;
    }
    
    // handle long button click
    if (btn_pressed && !btn_long_pressed && ((timer_now - btn_timer) >= LONG_PRESS_TIME)) {
        btn_long_pressed = true;
        switch (display_menu) {
            case MENU_POWER:
                enable_pwr_save_mode(!pwr_save_mode);
                display_data();
                if (pwr_save_mode) {
                    wheel_rotation_last_time = timer_now; // fixme
                }
                break;
            case MENU_LED:
                turn_led(!led_turned);
                display_data();
                break;
            default:
                turn_display(!display_turned); // todo turn on bluetooth
                break;
        }
    }
}

void detect_rotation() {
    wheel_rotated = true;
}

void calc_speed(unsigned long timer_now) {
    if (wheel_rotation_counter == WHEEL_ROTATION_MAX) {
        unsigned long interval = timer_now - wheel_rotation_start_time;
        float avg_interval = interval / WHEEL_ROTATION_MAX;

        uint16_t rpm = 60000 / avg_interval;
        if (rpm < WHEEL_RPM_MAX) {
            wheel_rpm = rpm;
            speed = wheel_rpm * 60 * wheel_length;
            if (speed >= max_speed) {
                max_speed = speed;
            }

            calc_avg_speed(speed);
            display_data();
        }

        wheel_rotation_counter = 0;
        wheel_rotation_start_time = 0;
    }
}

void loop() {
    if (sleep_mode) {
        sleep_mode = false;
        turn_display(true);
    }

    unsigned long timer_now = millis();

    handle_btn_click(digitalRead(BTN_PIN), timer_now);

    if (wheel_rotated) {
        wheel_rotated = false;
        wheel_rotation_last_time = timer_now;
        if (wheel_rotation_start_time == 0) {
            wheel_rotation_start_time = timer_now;
        }

        wheel_rotation_counter++;
        distance += wheel_length;
    }

    calc_speed(timer_now);
    
    // idle
    if (timer_now - wheel_rotation_last_time >= 3000) {
        if (speed != 0 || (0 < wheel_rotation_counter && wheel_rotation_counter < WHEEL_ROTATION_MAX)) {
            speed = 0;
            wheel_rpm = 0;
            wheel_rotation_counter = 0;
            wheel_rotation_start_time = 0;
            display_data();
        }
    }
    
    // sleep
    if (timer_now - wheel_rotation_last_time >= 20000) {
        wheel_rotation_last_time = 0;
        if (pwr_save_mode) {
            enable_sleep_mode();
        }
    }

    while (Serial.available() > 0) {
        int ch = Serial.read();
        if (isDigit(ch)) {
            serial_input += (char) ch;
        }
        if (ch == '\n') {
            int diameter = serial_input.toInt();
            Serial.println(String("Set new wheel diameter: ") + diameter);
            set_wheel_diameter(diameter);
            calc_wheel_length();
            serial_input = "";
        }
    }

    delay(2);
}
