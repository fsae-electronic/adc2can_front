#include <stdio.h>

#include "sys_common.h"
#include "sys_core.h"
#include "system.h"

#include "sci.h"
#include "rti.h"

#include "sensors.h"

#define CLOCK_RTI_HZ 10000000 
#define ADC_HZ 500
#define ADC_TICKS (CLOCK_RTI_HZ / ADC_HZ)

#define PLOT_HZ 50
#define PLOT_TICKS (CLOCK_RTI_HZ / PLOT_HZ)


char sciBuffer[64];

int main(void)
{
    _enable_interrupt_();

    sciInit();
    sciSetBaudrate(sciREG, 115200);

    init_sensors();


    rtiSetPeriod(rtiCOMPARE2, PLOT_TICKS);
    rtiEnableNotification(rtiNOTIFICATION_COMPARE2);
    


    rtiStartCounter(rtiCOUNTER_BLOCK1);




    while(1)
    {
        run_sensors();
    }

    return 0;
}

void rtiNotification(uint32 notification)
{
    if (notification == rtiNOTIFICATION_COMPARE0)
    {
    }
    if (notification == rtiNOTIFICATION_COMPARE1)
    {

    }
    if (notification == rtiNOTIFICATION_COMPARE2)
    {
        convert_data();
        send_data_to_serialplot();
    }
    
}
