#include "adc_wrapper.h"
#include "wheel_speed.h"


#define TPS1_CH ADC1
#define TPS2_CH ADC2
#define FRONT_BRAKE_CH ADC3
#define DIRECTION_CH ADC4

#define FRONT_LEFT_WHEEL_CH FREQ_MEASURE_CH1
#define FRONT_RIGHT_WHEEL_CH FREQ_MEASURE_CH2


typedef struct
{
    uint16_t tps1_value;
    uint16_t tps2_value;

    uint16_t tps1_min_value;
    uint16_t tps1_max_value;

    uint16_t tps2_min_value;
    uint16_t tps2_max_value;
} tps_data_t;

typedef struct
{
    uint16_t front_brake_value;
    uint16_t calibration_break_value;
} brake_data_t;

typedef struct
{
    uint16_t direction_value;
    uint16_t calibration_direction_value;
} direction_data_t;

#define THEETH_PER_WHEEL 20.0f
typedef struct
{
    uint16_t wheel_rpm;
} wheel_speed_data_t;


void init_sensors(void);
void run_sensors(void);
