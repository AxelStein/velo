#ifndef SPEEDOMETER_H
#define SPEEDOMETER_H

class Speedometer {
    public:
        /**
         * @param wheel_pin Hall sensor digital pin
         * @param wheel_length Wheel length in km
         * @param eeprom_total_distance Eeprom address of total distance
         */
        void init(uint8_t wheel_pin, float wheel_length, int eeprom_total_distance);

        /**
         * Detect wheel rotation and perform speed calculations
         */
        void detect_rotation();

        /**
         * @return true if speedometer enters the idle mode
         */
        bool idle();

        /**
         * Save total distance in EEPROM
         */
        void save_total_distance();

        /**
         * @return speed in km/h
         */
        float get_speed();

        /**
         * @return average speed in km/h
         */
        float get_avg_speed();

        /**
         * @return max speed in km/h
         */
        float get_max_speed();

        /**
         * @return current distance in km
         */
        float get_distance();

        /**
         * @return total distance in km
         */
        float get_total_distance();

        /**
         * @return current wheel rpm value
         */
        uint16_t get_wheel_rpm();

    private:
        /**
         * Calculate current speed, rpm and average speed after 5 wheel rotations
         */
        void calc_speed();

        /**
         * Calculate average speed
         */
        void calc_avg_speed();

        uint8_t WHEEL_PIN;
        float WHEEL_LENGTH;
        int EEPROM_TOTAL_DISTANCE;

        float speed;
        float avg_speed;
        float max_speed;
        float distance;
        float total_distance;
        uint16_t wheel_rpm;
        
        bool wheel_rotation_started;
        uint8_t wheel_rotation_counter;
        uint32_t wheel_rotation_start_time;
        uint32_t wheel_rotation_last_time;

        float period_distance;
        
        float speed_arr[4];
        uint8_t speed_arr_index;
};

#endif
