#include "adc_wrapper.h"
#include "freq_measure.h"


#define TPS1_CH ADC2
#define TPS2_CH ADC6
#define FRONT_BRAKE_CH ADC5
#define DIRECTION_CH ADC0

#define FRONT_LEFT_WHEEL_CH FREQ_MEASURE_CH1
#define FRONT_RIGHT_WHEEL_CH FREQ_MEASURE_CH2

#define THEETH_PER_WHEEL 20.0f
#define STEERING_MAX_ANGLE_DEG 3600.0f

// convert_data() execution frequency (RTI compare2)
#define DATA_PROCESS_HZ 150

// Implausibility detection (T.4.2 APPS / T.4.3 BSE)
#define IMPLAUSIBILITY_TIME_MS 100
#define IMPLAUSIBILITY_CYCLES ((IMPLAUSIBILITY_TIME_MS * DATA_PROCESS_HZ + 999) / 1000)

// Max deviation between TPS1 and TPS2 [%]
#define TPS_IMPLAUSIBILITY_DIFF 10

// TPS normal RAW range = calibrated [min, max] widened by this % of the calibrated span on each side.
// Outside this range = open circuit / short circuit
#define TPS_RAW_MARGIN_PCT 25

// Brake normal RAW range (12 bits over 5 V). Outside this range = open circuit / short circuit
// Min = calibration_brake_value (0 PSI) minus this % of the calibration value
#define BRAKE_RAW_MIN_MARGIN_PCT 25
// Fallback min when the brake is not calibrated (0.5 V -> 410)
#define BRAKE_RAW_MIN_UNCALIBRATED 410
// Max = 4.5 V -> 3686
#define BRAKE_RAW_MAX 3686


typedef struct
{
    uint16_t tps1_value;
    uint16_t tps2_value;

    uint16_t tps1_raw_value;
    uint16_t tps2_raw_value;

    uint16_t tps1_min_value;
    uint16_t tps1_max_value;

    uint16_t tps2_min_value;
    uint16_t tps2_max_value;
} tps_data_t;

typedef struct
{
    uint16_t front_brake_value;
    uint16_t front_brake_raw_value;
    uint16_t calibration_brake_value;
} brake_data_t;

typedef struct
{
    uint16_t direction_value;
    uint16_t direction_raw_value;
    uint16_t calibration_direction_value;
} direction_data_t;

typedef struct
{
    uint16_t wheel_rpm;
    float period_us;
} wheel_speed_data_t;

typedef struct
{
    tps_data_t tps_data;
    brake_data_t front_brake_data;
    direction_data_t direction_data;
    wheel_speed_data_t left_wheel_speed_data;
    wheel_speed_data_t right_wheel_speed_data;
} sensors_data_t;

enum calibration_cmd_id_t
{
    CAL_CMD_NONE = 0,
    CAL_CMD_TPS_0 = 1,
    CAL_CMD_TPS_100 = 2,
    CAL_CMD_LEFT_STEER = 3,
    CAL_CMD_CENTER_STEER = 4,
    CAL_CMD_RIGHT_STEER = 5,
    CAL_CMD_CURRENT_SENSORS = 6
};



extern sensors_data_t sensors_data;

extern uint8_t calibration_cmd_id;




void init_sensors(void);
void run_sensors(void);
void send_data_to_serial(void);
void send_data_to_can(void);
void convert_data(void);
void process_calibration_command(void);
