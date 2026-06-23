#ifndef WHEEL_SPEED_H_
#define WHEEL_SPEED_H_

#include <stdbool.h>

#include "ecap.h"

#define TEETH_PER_REV 60


typedef enum
{
    WHEEL_SPEED_FRONT_LEFT = 0,
    WHEEL_SPEED_FRONT_RIGHT,
    WHEEL_SPEED_COUNT
} wheel_speed_id_t;

typedef struct
{
    ecapBASE_t *ecap;
    uint32_t rpm;
    uint32_t cap_ticks;
    bool new_value;
} wheel_speed_t;

void init_wheel(void);
uint32_t get_wheel_rpm(wheel_speed_id_t wheel);


extern wheel_speed_t right_wheel;
extern wheel_speed_t left_wheel;


#endif /* WHEEL_SPEED_H_ */