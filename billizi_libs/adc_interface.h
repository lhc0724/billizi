#ifndef __ADC_INTERFACE__
#define __ADC_INTERFACE__

/* includes */
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"
#include "hal_adc.h"

#include "hci.h"
#include "gap.h"

#include "gpio_interface.h"
#include "timer_interface.h"

/* ADC Channel definition */
#define ADC_EXTERNAL    HAL_ADC_CHANNEL_0
#define ADC_SHUNT_R     HAL_ADC_CHANNEL_6

/* ADC resolution calculate unit */
#define REF125_UNIT (1.25/8191)
#define REFVDD_UNIT (3.3/8191)  
//current sensor shunt resistor value(0.1ohm Parallel)
#define SHUNT_R_VAL (0.025)     

/* ADC distribution resistance value */
#define EXT_RATIO_V  ((1000+43)/(43))
#define BATT_RATIO_V ((30+10)/(10)) 
#define SAMPLING_CNT  20

/* BATT Device reference voltage */
#define EXT_MIN_V   20
#define EXT_COMM_V  14 
#define MIN_BATT_V  (3.1)
#define MAX_BATT_V  (4.2)
#define SERVICE_BATT_V   (3.2)
#define BATT_REF_V (3.75)
#define BATT_CAPACITY   17760   //3.7[V] * 4800[mA] = 17760[mWh]

typedef enum _ADC_OPT {
    READ_BATT_SIDE,   //after shunt resistor, battery side
    READ_INDUCTOR_SIDE,   //before shunt resistor, inductor side
    READ_EXT,
    READ_CURR_DISCHG,
    READ_CURR_CHG
}adc_option_t;

void get_gparam_calib(void *p_value);
void setup_calib_value(uint8 self_calib, uint16 adc_ref);

void adc_init();
uint16 read_adc(adc_option_t adc_opt);
uint16 read_adc_sampling(uint8 samp_cnt, adc_option_t adc_opt);
float read_voltage(adc_option_t adc_opt);
float read_voltage_sampling(uint8 samp_cnt, adc_option_t adc_opt);
uint16 read_current(adc_option_t curr_direction);

void open_adc_driver(adc_option_t adc_opt);
void close_adc_driver();

#endif