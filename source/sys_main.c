#include <stdio.h>

#include "sys_common.h"
#include "sys_core.h"
#include "system.h"

#include "sci.h"
#include "rti.h"

#include "wheel_speed.h"

char sciBuffer[64];

int main(void)
{
    _enable_interrupt_();

    sciInit();
    sciSetBaudrate(sciREG, 115200);

    init_wheel();



    while(1)
    {
        if(left_wheel.new_value)
        {
            uint32_t rpm = get_wheel_rpm(WHEEL_SPEED_FRONT_LEFT);
            uint16_t len = sprintf((char *)sciBuffer, "Left Wheel RPM: %lu\n", rpm);
            sciSend(sciREG, len, sciBuffer);
        }
        if(right_wheel.new_value)
        {
            uint32_t rpm = get_wheel_rpm(WHEEL_SPEED_FRONT_RIGHT);
            uint16_t len = sprintf((char *)sciBuffer, "Right Wheel RPM: %lu\n", rpm);
            sciSend(sciREG, len, sciBuffer);
        }
    }

    return 0;
}

