#include "sensors.h"

#include "ti_fee.h"

sensors_data_t sensors_data;


//Calibration values to save on EPROM
struct EEPROM_data{
    union
    {
        uint8_t raw[12];
        struct
        {
            uint16_t magic;

            uint16_t tps1_min_value;
            uint16_t tps1_max_value;

            uint16_t tps2_min_value;
            uint16_t tps2_max_value;

            uint16_t calibration_brake_value;
            uint16_t calibration_direction_value;
        } values;
    };
};

static const uint16_t EEPROM_MAGIC = 0xAAAA;

static uint16_t period_us_to_rpm(float period_us)
{
    float rpm;

    if (period_us <= 0.0f)
    {
        return 0U;
    }

    rpm = 60000000.0f / (period_us * THEETH_PER_WHEEL);

    if (rpm >= 65535.0f)
    {
        return 65535U;
    }

    return (uint16_t)rpm;
}


/********************************
 * Static Functions
 *******************************/

bool load_calibration_data_from_eeprom()
{
    struct EEPROM_data e;

    //Read the data from EEPROM
    while(TI_Fee_GetStatus(0) != IDLE)
    {
        TI_Fee_MainFunction();
    }
    TI_Fee_ReadSync(1, 0, e.raw, sizeof(struct EEPROM_data));

    if(e.values.magic != EEPROM_MAGIC)
    {
        return false;
    }

    sensors_data.tps_data.tps1_min_value = e.values.tps1_min_value;
    sensors_data.tps_data.tps1_max_value = e.values.tps1_max_value;

    sensors_data.tps_data.tps2_min_value = e.values.tps2_min_value;
    sensors_data.tps_data.tps2_max_value = e.values.tps2_max_value;

    sensors_data.front_brake_data.calibration_brake_value = e.values.calibration_brake_value;

    sensors_data.direction_data.calibration_direction_value = e.values.calibration_direction_value;

    return true;
}

void save_calibration_data_to_eeprom(void)
{
    while(TI_Fee_GetStatus(0) != IDLE)
    {
        TI_Fee_MainFunction();
    }
    struct EEPROM_data calibration_data;
    calibration_data.values.magic = EEPROM_MAGIC;

    calibration_data.values.tps1_min_value = sensors_data.tps_data.tps1_min_value;
    calibration_data.values.tps1_max_value = sensors_data.tps_data.tps1_max_value;

    calibration_data.values.tps2_min_value = sensors_data.tps_data.tps2_min_value;
    calibration_data.values.tps2_max_value = sensors_data.tps_data.tps2_max_value;

    calibration_data.values.calibration_brake_value = sensors_data.front_brake_data.calibration_brake_value;

    calibration_data.values.calibration_direction_value = sensors_data.direction_data.calibration_direction_value;

    TI_Fee_WriteAsync(1, calibration_data.raw);

    while(TI_Fee_GetStatus(0) != IDLE)
    {
        TI_Fee_MainFunction();
    }
}

void init_sensors(void)
{
    sensors_data.tps_data.tps1_value = 0;
    sensors_data.tps_data.tps2_value = 0;

    sensors_data.tps_data.tps1_min_value = 0;
    sensors_data.tps_data.tps1_max_value = 0;

    sensors_data.tps_data.tps2_min_value = 0;
    sensors_data.tps_data.tps2_max_value = 0;

    sensors_data.front_brake_data.front_brake_value = 0;
    sensors_data.front_brake_data.calibration_brake_value = 0;

    sensors_data.direction_data.direction_value = 0;
    sensors_data.direction_data.calibration_direction_value = 0;

    sensors_data.left_wheel_speed_data.wheel_rpm = 0;
    sensors_data.right_wheel_speed_data.wheel_rpm = 0;

    //Load calibration and all that stuff
    //TODO: Load calibration data from EEPROM
    //
    init_adc();
    init_freq_measure();

}


void run_sensors(void)
{
    process_adc_data();
}

#include "sci.h"
void send_data_to_serialplot(void)
{
    uint16_t start = 0xAA55;
    sciSend(sciREG, 2, (uint8_t*)&start);
    sciSend(sciREG, 2, (uint8_t*)&sensors_data.tps_data.tps1_value);
    sciSend(sciREG, 2, (uint8_t*)&sensors_data.tps_data.tps2_value);
    sciSend(sciREG, 2, (uint8_t*)&sensors_data.front_brake_data.front_brake_value);
    sciSend(sciREG, 2, (uint8_t*)&sensors_data.direction_data.direction_value);
    sciSend(sciREG, 2, (uint8_t*)&sensors_data.left_wheel_speed_data.wheel_rpm);
    sciSend(sciREG, 2, (uint8_t*)&sensors_data.right_wheel_speed_data.wheel_rpm);
}

void convert_data(void)
{
    sensors_data.tps_data.tps1_value = adc_data[TPS1_CH].adc_value;
    sensors_data.tps_data.tps2_value = adc_data[TPS2_CH].adc_value;

    sensors_data.front_brake_data.front_brake_value = adc_data[FRONT_BRAKE_CH].adc_value;
    sensors_data.direction_data.direction_value = adc_data[DIRECTION_CH].adc_value;

    float left_period_us = get_period_us_measure(FRONT_LEFT_WHEEL_CH);
    float right_period_us = get_period_us_measure(FRONT_RIGHT_WHEEL_CH);

    sensors_data.left_wheel_speed_data.wheel_rpm = period_us_to_rpm(left_period_us);
    sensors_data.right_wheel_speed_data.wheel_rpm = period_us_to_rpm(right_period_us);
}


