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

/*

EXT_V mode : 외부 전압 상태

NO_VOLT : 무전압, 키오스크 외부 혹은 내부에 있으면서 프로토콜 중간 스테이트
COMM_VOLT : 통신전압
CHRGED_VOLT : 충전되는 전압

*/

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
#define BATT_RATIO_V  4     //(30+10)/10 = 4
#define SAMPLING_CNT  5

/* BATT Device reference voltage */
#define EXT_MIN_V   20 // 외부 충전 전압으로 인정할  최소값
#define EXT_COMM_V  8  // 통신 상태로 인정할 전압값
#define EXT_ZERO_V  0  //  외부 전압 없음
#define MIN_BATT_V  (3.1)
#define MAX_BATT_V  (4.2)
#define SERVICE_BATT_V   (3.2)
#define BATT_REF_V  (3.75)
#define BATT_CAPACITY   17760   //3.7[V] * 4800[mA] = 17760[mWh]
/*
READ_EXT 전압:ADC값
17.945V : 4900
24.35V  : 6735
1.12V   : 165

READ_BATT_SIDE
3.850v : 6585 
3.743v : 6403 
3.905v : 6678
*/
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
uint16 read_voltage_uint16(adc_option_t adc_opt);
float read_voltage_sampling(uint8 samp_cnt, adc_option_t adc_opt);
uint16 read_current(adc_option_t curr_direction);

uint8 ext_voltage_analysis(float voltage);

void open_adc_driver(adc_option_t adc_opt);
void close_adc_driver();
uint8 ext_voltage_result(void);

#endif
