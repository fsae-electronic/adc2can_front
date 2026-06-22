#include "sys_common.h"
#include "sys_core.h"
#include "sci.h"
#include "rti.h"
#include <stdio.h>

#include "sensors.h"



void send_data_uart()
{
    uint16_t start_frame = 0xFFFF;
    sciSend(sciREG, 2, (uint8 *)&start_frame); // Enviar un byte para indicar el inicio del frame


    //Send datos por SCI
    for (int i = 0; i < ADC_NUM_CHANNELS; i++)
    {
        uint16_t value = adc_data[i].adc_value;
        sciSend(sciREG, 2, (uint8 *)&value);    // Enviar el valor del ADC
    }
}



int main(void)
{
    _enable_interrupt_();

    sciInit();
    sciSetBaudrate(sciREG, 115200);
    init_sensors();

    while(1)
    {
        process_adc_data();
        send_data_uart();

    }

    return 0;
}




void rtiNotification(uint32 notification)
{
    //1 seg
    if (notification == rtiNOTIFICATION_COMPARE0)
    {

    }
}



