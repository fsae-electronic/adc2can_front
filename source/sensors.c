#include "sensors.h"

#include "ti_fee.h"
#include "sci.h"
#include "can.h"

sensors_data_t sensors_data;

uint8_t calibration_cmd_id = CAL_CMD_NONE;


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

bool load_e_from_eeprom()
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

void save_e_to_eeprom(void)
{
    while(TI_Fee_GetStatus(0) != IDLE)
    {
        TI_Fee_MainFunction();
    }
    struct EEPROM_data e;
    e.values.magic = EEPROM_MAGIC;

    e.values.tps1_min_value = sensors_data.tps_data.tps1_min_value;
    e.values.tps1_max_value = sensors_data.tps_data.tps1_max_value;

    e.values.tps2_min_value = sensors_data.tps_data.tps2_min_value;
    e.values.tps2_max_value = sensors_data.tps_data.tps2_max_value;

    e.values.calibration_brake_value = sensors_data.front_brake_data.calibration_brake_value;

    e.values.calibration_direction_value = sensors_data.direction_data.calibration_direction_value;

    TI_Fee_WriteAsync(1, e.raw);

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
    sensors_data.front_brake_data.front_brake_raw_value = 0;
    sensors_data.front_brake_data.calibration_brake_value = 0;

    sensors_data.direction_data.direction_value = 0;
    sensors_data.direction_data.direction_raw_value = 0;
    sensors_data.direction_data.calibration_direction_value = 0;

    sensors_data.left_wheel_speed_data.wheel_rpm = 0;
    sensors_data.left_wheel_speed_data.period_us = 0.0f;
    sensors_data.right_wheel_speed_data.wheel_rpm = 0;
    sensors_data.right_wheel_speed_data.period_us = 0.0f;

    //Load calibration values from EEPROM
    load_e_from_eeprom();


    init_adc();
    init_freq_measure();

}


void send_data_to_serial(void)
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

void send_data_to_can(void)
{
    uint8_t tps_data[8];
    tps_data[0] = (uint8_t)(sensors_data.tps_data.tps1_value & 0xFF);
    tps_data[1] = (uint8_t)((sensors_data.tps_data.tps1_value >> 8) & 0xFF);
    tps_data[2] = (uint8_t)(sensors_data.tps_data.tps2_value & 0xFF);
    tps_data[3] = (uint8_t)((sensors_data.tps_data.tps2_value >> 8) & 0xFF);
    canTransmit(canREG1, canMESSAGE_BOX2,(uint8_t*)&sensors_data);

    uint8_t front_data[8];
    front_data[0] = (uint8_t)(sensors_data.left_wheel_speed_data.wheel_rpm & 0xFF);
    front_data[1] = (uint8_t)((sensors_data.left_wheel_speed_data.wheel_rpm >> 8) & 0xFF);
    front_data[2] = (uint8_t)(sensors_data.right_wheel_speed_data.wheel_rpm & 0xFF);
    front_data[3] = (uint8_t)((sensors_data.right_wheel_speed_data.wheel_rpm >> 8) & 0xFF);
    front_data[4] = (uint8_t)(sensors_data.direction_data.direction_value & 0xFF);
    front_data[5] = (uint8_t)((sensors_data.direction_data.direction_value >> 8) & 0xFF);
    front_data[6] = (uint8_t)(sensors_data.front_brake_data.front_brake_value & 0xFF);
    front_data[7] = (uint8_t)((sensors_data.front_brake_data.front_brake_value >> 8) & 0xFF);
    canTransmit(canREG1, canMESSAGE_BOX3, front_data);
}

void convert_data(void)
{
    //ADC raw value
    sensors_data.tps_data.tps1_raw_value = adc_data[TPS1_CH].adc_value;
    sensors_data.tps_data.tps2_raw_value = adc_data[TPS2_CH].adc_value;
    sensors_data.front_brake_data.front_brake_raw_value = adc_data[FRONT_BRAKE_CH].adc_value;
    sensors_data.direction_data.direction_raw_value = adc_data[DIRECTION_CH].adc_value;

    // TPS Conversion
    // TPS1
    if(sensors_data.tps_data.tps1_raw_value > sensors_data.tps_data.tps1_min_value)
        sensors_data.tps_data.tps1_value = 0;
    else if(sensors_data.tps_data.tps1_raw_value < sensors_data.tps_data.tps1_max_value)
        sensors_data.tps_data.tps1_value = 100;
    else
        sensors_data.tps_data.tps1_value = (uint16_t)(((float)(sensors_data.tps_data.tps1_min_value - sensors_data.tps_data.tps1_raw_value)
                                                     / (float)(sensors_data.tps_data.tps1_min_value - sensors_data.tps_data.tps1_max_value)) * 100.0f);

    // TPS2
    if(sensors_data.tps_data.tps2_raw_value < sensors_data.tps_data.tps2_min_value)
        sensors_data.tps_data.tps2_value = 0;
    else if(sensors_data.tps_data.tps2_raw_value > sensors_data.tps_data.tps2_max_value)
        sensors_data.tps_data.tps2_value = 100;
    else
        sensors_data.tps_data.tps2_value = (uint16_t)(((float)(sensors_data.tps_data.tps2_raw_value - sensors_data.tps_data.tps2_min_value)
                                                     / (float)(sensors_data.tps_data.tps2_max_value - sensors_data.tps_data.tps2_min_value)) * 100.0f);       
                                                     
    // Brake Conversion
	// P [PSI] = 400*(V - 0.5V)
	// V = raw_value/4095 * 5.0
    if(sensors_data.front_brake_data.front_brake_raw_value < sensors_data.front_brake_data.calibration_brake_value)
        sensors_data.front_brake_data.front_brake_value = 0;
    else
        sensors_data.front_brake_data.front_brake_value =
            (uint16_t)(400.0f * ((((float)sensors_data.front_brake_data.front_brake_raw_value / 4095.0f) * 5.0f) - 0.5f));
    // Freq raw value
    sensors_data.left_wheel_speed_data.period_us = get_period_us_measure(FRONT_LEFT_WHEEL_CH);
    sensors_data.right_wheel_speed_data.period_us = get_period_us_measure(FRONT_RIGHT_WHEEL_CH);

    sensors_data.left_wheel_speed_data.wheel_rpm = period_us_to_rpm(sensors_data.left_wheel_speed_data.period_us);
    sensors_data.right_wheel_speed_data.wheel_rpm = period_us_to_rpm(sensors_data.right_wheel_speed_data.period_us);

    // Direction Conversion
    sensors_data.direction_data.direction_value = (uint16_t)(((float)(sensors_data.direction_data.direction_raw_value - sensors_data.direction_data.calibration_direction_value)
                                                     / (float)(4095.0f)) * STEERING_MAX_ANGLE_DEG) + 180;
}

void process_calibration_command(void)
{
    if(calibration_cmd_id == CAL_CMD_NONE)
    {
        return;
    }

    switch(calibration_cmd_id)
    {
        case CAL_CMD_TPS_0:
            sensors_data.tps_data.tps1_min_value = sensors_data.tps_data.tps1_raw_value;
            sensors_data.tps_data.tps2_min_value = sensors_data.tps_data.tps2_raw_value;
            sensors_data.front_brake_data.calibration_brake_value = sensors_data.front_brake_data.front_brake_raw_value;
            break;
        case CAL_CMD_TPS_100:
            sensors_data.tps_data.tps1_max_value = sensors_data.tps_data.tps1_raw_value;
            sensors_data.tps_data.tps2_max_value = sensors_data.tps_data.tps2_raw_value;
            break;
        case CAL_CMD_CENTER_STEER:
            sensors_data.direction_data.calibration_direction_value = sensors_data.direction_data.direction_raw_value;
            break;

        default:
            break;
    }

    save_e_to_eeprom();
    calibration_cmd_id = CAL_CMD_NONE;
}
