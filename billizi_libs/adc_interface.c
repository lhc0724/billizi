#include "adc_interface.h"

static float convert_voltage(uint16 adc_org, adc_option_t adc_opt);
float g_calib;


void get_gparam_calib(void *p_value) 
{
    *((float *)p_value) = g_calib;
}

void setup_calib_value(uint8 calib_opt, uint16 calib_value)
{   
    float adc_voltage;

    adc_voltage = calib_value * REF125_UNIT * BATT_RATIO_V;
    if (calib_opt) {
        /* set self calibration */
        g_calib = MAX_BATT_V/adc_voltage;    
    }else {
        /* set reference voltage calibration */
        g_calib = BATT_REF_V/adc_voltage;    
    }
}

void adc_init()
{
    g_calib = 0;
	HalAdcSetReference(HAL_ADC_REF_125V);
}

uint16 read_adc(adc_option_t adc_opt) 
{
    uint16 adc_val;

    switch(adc_opt) {
        case READ_BATT_SIDE:
        case READ_INDUCTOR_SIDE:
            adc_val = HalAdcRead(ADC_SHUNT_R, HAL_ADC_RESOLUTION_14);
            break;
        case READ_EXT:
            adc_val = HalAdcRead(ADC_EXTERNAL, HAL_ADC_RESOLUTION_14);
            break;
        default:
            adc_val = 0;
            break;
    }

    return adc_val;
}

uint16 read_adc_sampling(uint8 samp_cnt, adc_option_t adc_opt)
{
    uint8 i;
    uint32 adc_tmp = 0;

    open_adc_driver(adc_opt);

    for(i = 0; i < samp_cnt; i++) {
        adc_tmp += read_adc(adc_opt);
    }
    close_adc_driver();

    adc_tmp = adc_tmp/samp_cnt;

    return (uint16)adc_tmp;
}

float read_voltage(adc_option_t adc_opt)
{
    float f_voltage = 0;
    uint16 adc_org;

    open_adc_driver(adc_opt); 

    adc_org = read_adc(adc_opt);
    f_voltage = convert_voltage(adc_org, adc_opt);

    close_adc_driver();

    return f_voltage;
}

float read_voltage_sampling(uint8 samp_cnt, adc_option_t adc_opt)
{
    float f_voltage;
    uint16 adc_samp;

    adc_samp = read_adc_sampling(samp_cnt, adc_opt);
    f_voltage = convert_voltage(adc_samp, adc_opt);

    return f_voltage;
}

uint8 ext_voltage_analysis(float voltage)
{
    if (voltage >= EXT_MIN_V) {
        return EXT_MIN_V;
    } else if (voltage >= EXT_COMM_V) {
        return EXT_COMM_V;
    }

    return 0;
}

uint16 read_current(adc_option_t curr_direction) 
{
    float res_curr = 0;
    uint16 adc_values[2];

    adc_values[0] = read_adc_sampling(2, READ_BATT_SIDE);
    adc_values[1] = read_adc_sampling(2, READ_INDUCTOR_SIDE);

    switch (curr_direction) {
        case READ_CURR_CHG: 
            if(adc_values[1] > adc_values[0]) {
                res_curr = adc_values[1] - adc_values[0];
            }
            break;
        case READ_CURR_DISCHG:
            if (adc_values[0] > adc_values[1]) {
                res_curr = adc_values[0] - adc_values[1];
            }
            break;
        default:
            //do nothing
            break;
    }

    res_curr = ((res_curr * REF125_UNIT * BATT_RATIO_V) / SHUNT_R_VAL) * 1000;

    return (uint16)res_curr;
}

void close_adc_driver()
{
    IO_ADC_BATT_SIDE = 0;
    IO_ADC_INDUCTOR_SIDE = 0;
}

void open_adc_driver(adc_option_t adc_opt)
{
    switch(adc_opt) {
        case READ_BATT_SIDE:
            IO_ADC_BATT_SIDE = 1;
            break;
        case READ_INDUCTOR_SIDE:
            IO_ADC_INDUCTOR_SIDE = 1;
            break;
        default:
            //do not working option
            break;
    }
    if (adc_opt != READ_EXT) {
        delay_us(50);
    }
}

/* local function group */

static float convert_voltage(uint16 adc_org, adc_option_t adc_opt) 
{
    float f_voltage;
    switch (adc_opt) {
        case READ_BATT_SIDE:
        case READ_INDUCTOR_SIDE:
            if (g_calib == 0) {
                f_voltage = adc_org * REF125_UNIT * BATT_RATIO_V;
            } else {
                f_voltage = adc_org * REF125_UNIT * BATT_RATIO_V * g_calib;
            }
            break;
        case READ_EXT:
            f_voltage = adc_org * REF125_UNIT * EXT_RATIO_V;
            break;
    }
    return f_voltage;
}