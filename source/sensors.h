#include "adc_wrapper.h"
#include "freq_measure.h"


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
    uint16_t calibration_brake_value;
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

typedef struct
{
    tps_data_t tps_data;
    brake_data_t front_brake_data;
    direction_data_t direction_data;
    wheel_speed_data_t left_wheel_speed_data;
    wheel_speed_data_t right_wheel_speed_data;
} sensors_data_t;


extern sensors_data_t sensors_data;




void init_sensors(void);
void run_sensors(void);
void send_data_to_serialplot(void);
void convert_data(void);
