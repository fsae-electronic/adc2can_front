#include "sensors.h"

#include "ti_fee.h"

sensors_data_t sensors_data;


//Calibration values to save on EPROM
struct EEPROM_data
{
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

            uint16_t calibration_break_value;
            uint16_t calibration_direction_value;
        } values;
    };
};

static const uint16_t EEPROM_MAGIC = 0xAAAA;


/********************************
 * Static Functions
 *******************************/

bool load_calibration_data_from_eeprom(void)
{
    EEPROM_data e;

    //Read the data from EEPROM
    while(TI_Fee_GetStatus(0) != IDLE)
    {
        TI_Fee_MainFunction();
    }
    TI_Fee_ReadSync(1, 0, e.raw, sizeof(EEPROM_data));

    if(e.values.magic != EEPROM_MAGIC)
    {
        return false;
    }

    sensors_data.tps_data.tps1_min_value = e.values.tps1_min_value;
    sensors_data.tps_data.tps1_max_value = e.values.tps1_max_value;

    sensors_data.tps_data.tps2_min_value = e.values.tps2_min_value;
    sensors_data.tps_data.tps2_max_value = e.values.tps2_max_value;

    sensors_data.front_brake_data.calibration_break_value = e.values.calibration_break_value;

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

    calibration_data.values.calibration_break_value = sensors_data.front_brake_data.calibration_break_value;

    calibration_data.values.calibration_direction_value = sensors_data.direction_data.calibration_direction_value;

    TI_Fee_WriteAsync(1, 0, calibration_data.raw, sizeof(EEPROM_data));
    
    while(TI_Fee_GetStatus(0) != IDLE)
    {
        TI_Fee_MainFunction();
    }
}
