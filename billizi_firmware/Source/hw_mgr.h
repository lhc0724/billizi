#ifndef __HW_MANAGER__
#define __HW_MANAGER__

#include "adc_interface.h"
#include "gpio_interface.h"
#include "timer_interface.h"
#include "flash_interface.h"

#define LM75_ADDR   0x48

#define ERR_BROKEN_CABLE    0x01
#define ERR_FLASH_MEMS      0x02
#define ERR_TEMP_OVER       0x04
#define ERR_IMPACT_OVER     0x08
#define ERR_LOSS_BATTERY    0x10
#define ERR_COMMUNICATION   0x20

typedef union _ctrl_flag
{   
    struct {    
        //user service flag group
        uint8 serv_en : 1;
        /** @param certification - 추후 배터리 인증시 플레그 활성화, 배터리 동작유무에 영향 O */
        uint8 certification : 1;
        uint8 need_comm : 1;
        uint8 use_conn : 1;    
        uint8 logging : 1;

        //factory init flag group
        uint8 factory_setup : 1;
        uint8 self_calib : 1;
        uint8 ref_calib : 1;    

        //state flag group
        uint8 trans_status : 1;  //change state flag.

        uint8 abnormal : 6;     
        //total 16 bits
    };
    uint16 flag_all;
}Control_flag_t;

typedef struct _BATT_STATUS {
    float batt_v;
    uint16 current;
    uint16 pwr_consum;
    uint16 left_cap;
}batt_info_t;

typedef struct _SENSOR_STATUS {
    uint16 impact_cnt;
    uint16 temperature;
}sensor_info_t;

static int16 calc_i2c_temperature(uint8 * i2c_data);

int16 read_temperature();
uint8 check_cable_status();

void sensor_status_init(sensor_info_t *p_sensor);
void get_batt_status(batt_info_t *p_batt_status);

#endif