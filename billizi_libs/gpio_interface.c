#include "gpio_interface.h"
#include "timer_interface.h"

void setup_pin()
{
	//pin setting
	//PnSEL: 1:not use pio 0:use gpio
	//PnINP: 1:not use pullup 0:use pullup
	//PnDIR: 1:gpio output 0:gpio input
    //P2INP Bit 5, 6, 7 is port0, 1, 2 input mode control register, 0 : pullup, 1: pulldown

    P0 = 0;
    P0SEL = BIT0|BIT6;
    P0INP = 0xFF;
    //P0DIR = BIT4|BIT5|BIT7;    //P_(4,5,7) output, use vib_motor
    P0DIR = BIT4|BIT5;          //P_(4,5) output, P_(3,7) input, use vib_sensor

	P1SEL = BIT6|BIT7;           //using uart
    P1INP = 0xFF;
    P1DIR = BIT0|BIT2|BIT3|BIT4|BIT5|BIT6;  

    POWER_EN = 1;

    P2SEL &= ~(BIT0);
    P2SEL |= BIT3;
    P2DIR = 0;
    
    P2INP = BIT1;

    EN_CONN_USB  = 0;
    EN_CONN_RETR = 0;
    CHG_EN       = 0;
    RETR_TEST_EN = 0;
    IO_ADC_BATT_SIDE   = 0;
    IO_ADC_INDUCTOR_SIDE   = 0;

	//HalAdcSetReference(HAL_ADC_REF_125V);
}

bool detect_use_cable()
{
    if(RETR_PUSH_RIGHT && RETR_PUSH_LEFT) {
        /* use billizi cable */
        return TRUE;
    }
    return FALSE;
}

void uart_disable()
{
    P1SEL &= (0xFF & ~(BIT6|BIT7));
    P1DIR |= BIT6;  

    P1_6 = 0;
}

void uart_enable()
{
	P1SEL |= BIT6|BIT7;  
}

void charge_enable()
{
    uart_disable();
    CHG_EN = 1;
}

void charge_disable()
{
    CHG_EN = 0;
}

void discharge_enable(eDischg_t direction)
{
    switch(direction) {
        case DISCHG_BLZ_CONN:
            EN_CONN_USB = 0;
            EN_CONN_RETR = 1;
            break;
        case DISCHG_USB_A:
            EN_CONN_RETR = 0;
            EN_CONN_USB = 1;
            break;
    }
}

void discharge_disable()
{
    EN_CONN_USB = 0;
    EN_CONN_RETR = 0;
}

void hw_power_off()
{
    POWER_EN = 0;
}