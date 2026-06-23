#include "wheel_speed.h"

#include "ecap.h"
#include "sys_common.h"
#include "sys_core.h"
#include "system.h"

wheel_speed_t right_wheel;
wheel_speed_t left_wheel;


void init_wheel(void)
{
    // Initialize ECAP modules for wheel speed measurement
    left_wheel.ecap = ecapREG1;  // ECAP1 for front left wheel
    right_wheel.ecap = ecapREG2; // ECAP2 for front right wheel

    left_wheel.rpm = 0;
    left_wheel.cap_ticks = 0;
    left_wheel.new_value = false;

    right_wheel.rpm = 0;
    right_wheel.cap_ticks = 0;
    right_wheel.new_value = false;
    


    ecapInit();

    // Configure ECAP1 for front left wheel
    ecapSetCaptureEvent1(ecapREG1, FALLING_EDGE, RESET_ENABLE);
    ecapSetCaptureMode(ecapREG1, CONTINUOUS, CAPTURE_EVENT1);
    ecapStartCounter(ecapREG1);
    ecapEnableCapture(ecapREG1);
    ecapEnableInterrupt(ecapREG1, ecapInt_CEVT1);

    // Configure ECAP2 for front right wheel
    ecapSetCaptureEvent1(ecapREG2, FALLING_EDGE, RESET_ENABLE);
    ecapSetCaptureMode(ecapREG2, CONTINUOUS, CAPTURE_EVENT1);
    ecapStartCounter(ecapREG2);
    ecapEnableCapture(ecapREG2);
    ecapEnableInterrupt(ecapREG2, ecapInt_CEVT1);
}

uint32_t get_wheel_rpm(wheel_speed_id_t wheel)
{
    if (wheel == WHEEL_SPEED_FRONT_LEFT)
    {
        left_wheel.new_value = false;
        return left_wheel.rpm;
    }
    else if (wheel == WHEEL_SPEED_FRONT_RIGHT)
    {
        right_wheel.new_value = false;
        return right_wheel.rpm;
    }
    else
    {
        return 0; // Invalid wheel ID
    }
}

void ecapNotification(ecapBASE_t *ecap, uint16 flags)
{
    uint32_t capture_value;
    float period_s;

    if (ecap == left_wheel.ecap)
    {
        capture_value = ecapGetCAP1(ecap);
        period_s = ((float)capture_value / VCLK4_FREQ) / 1000000.0;
        left_wheel.cap_ticks = capture_value;
        left_wheel.rpm = (uint32_t)(60.0 / (period_s * TEETH_PER_REV));
        left_wheel.new_value = true;
    }
    else if (ecap == right_wheel.ecap)
    {
        capture_value = ecapGetCAP1(ecap);
        period_s = ((float)capture_value / VCLK4_FREQ) / 1000000.0;
        right_wheel.cap_ticks = capture_value;
        right_wheel.rpm = (uint32_t)(60.0 / (period_s * TEETH_PER_REV));
        right_wheel.new_value = true;
    }
}



