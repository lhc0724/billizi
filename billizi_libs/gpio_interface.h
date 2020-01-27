#ifndef __GPIO_INTERFACE__
#define __GPIO_INTERFACE__

/* includes */
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"

#include "hci.h"
#include "gap.h"

/* GPIO definition */
/* batt upper snu v1.0.0 gpio allocation. */
/* definition of battery control gpio register */
#define RETR_PUSH_LEFT     P1_1 //INPUT
#define RETR_PUSH_RIGHT    P2_0 //INPUT
#define RETR_CABLE_STATUS  P0_3 //INPUT
#define RETR_TEST_EN       P0_4 //OUTPUT

#define EN_CONN_USB        P1_2 //OUTPUT
#define EN_CONN_RETR       P0_5 //OUTPUT
#define CHG_EN             P1_5 //OUTPUT
#define TXD_PIO            P1_6 //UART Txd pin

#define IO_ADC_INDUCTOR_SIDE    P1_0 //OUTPUT, adc drive, before shunt resistor
#define IO_ADC_BATT_SIDE        P1_4 //OUTPUT, adc drive, after shunt resistor
#define POWER_EN           P1_3 //OUTPUT

#define VIB_SENSOR         P0_7 //INPUT
//#define VIB_MOTOR          P0_7 //OUTPUT

/* definition for bit */
#define BIT7        0x80
#define BIT6        0x40
#define BIT5        0x20
#define BIT4        0x10
#define BIT3        0x08
#define BIT2        0x04
#define BIT1        0x02
#define BIT0        0x01

typedef enum _DISCHG_OPTION {
    DISCHG_BLZ_CONN, DISCHG_USB_A
}eDischg_t;

void setup_pin();
void hw_power_off();

void uart_enable();
void uart_disable();
void charge_enable();
void charge_disable();
void discharge_enable(eDischg_t conn_type);
void discharge_disable();

bool detect_use_cable();

#endif