#include "hw_mgr.h"
#include "hal_i2c.h"


uint8 check_cable_status()
{
    uint8 cable_status = 0; 

    RETR_TEST_EN = 1;
    EN_CONN_RETR = 1;
    
    delay_us(100);
    if(!RETR_CABLE_STATUS) {
        //broken cable
        cable_status = 1;
    }

    RETR_TEST_EN = 0;
    EN_CONN_RETR = 0;

    return cable_status;
}

void sensor_status_init(sensor_info_t *p_sensor)
{
    p_sensor->impact_cnt = 0;
    p_sensor->temperature = 0;
}

void get_batt_status(batt_info_t *p_batt_status)
{
    float f_tmpdata;

    p_batt_status->batt_v = read_voltage(READ_BATT_SIDE);
    p_batt_status->current = read_current(READ_CURR_DISCHG);
    
    //current power consumption [mW]
    f_tmpdata = p_batt_status->batt_v * (p_batt_status->current);
    
    //calc to electric power [mW/h]
    f_tmpdata = f_tmpdata/3600;
    
    p_batt_status->left_cap -= (uint16)f_tmpdata;
}

int16 read_temperature()
{
    uint8 temp_arr[2] = { 0 };
    uint8 read_size = 2;

    HalI2CInit(i2cClock_33KHZ); //i2c interface drive clock is 33Khz
    HalI2CRead(LM75_ADDR, read_size, temp_arr);

    //temperature value is 
    return calc_i2c_temperature(temp_arr);
}

static int16 calc_i2c_temperature(uint8 * i2c_data)
{
    int16 res_temp;
    uint8 temp_level = 1;
    uint8 i;

    for(i = 0; i < 7; i++) {
        if((i2c_data[0] >> i) & 1) {
            res_temp += temp_level;
            //f_temp += temp_level;
        }
        temp_level = temp_level * 2;
    }

    res_temp = res_temp * 10;

    if((i2c_data[1] >> 7) & 1) {
        res_temp += 5;
        //f_temp += 0.5;
    }

    if((i2c_data[0] >> 7) & 1) {
        res_temp *= -1;
        //f_temp *= -1;
    }

    return res_temp;
}