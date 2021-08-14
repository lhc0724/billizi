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
#define ERR_EXCEED_IMPACT_LIMIT     0x08
#define ERR_LOSS_BATTERY    0x10
#define ERR_COMMUNICATION   0x20

typedef union _ctrl_flag
{   
    struct {    
        //user service flag group
        uint8 serv_en : 1; 		// 사용자 서비스 활성, 0인 경우 방전 금지함.
        uint8 need_comm : 1; 	// 충전선 통신 여부
        uint8 use_conn : 1;		// 방전할 때 빌리지 코넥터 사용 여부
        uint8 logging : 1; 		// 로그를 해야함
        uint8 batt_dischg : 1; 	// 방전 여부

        //factory init flag group
        uint8 just_after_fw_download : 1; 	//1 : 펌웨어 다운로드 직후여서 코넥터 종류와 ADC보정이 안된 상태, 
											//0 : 배터리 설정이 끝난 상태 
        uint8 self_calib : 1; // 1:자가 조정 후
        uint8 ref_calib : 1;  // 1:외부 기준에 의한 조정

        uint8 abnormal : 8;     
        //total 15 bits
    };
    uint16 flag_all;
}Control_flag_t;

typedef struct _BATT_STATUS {
    float batt_v;
    uint16 current;
    uint16 hysteresis_cnt;
}batt_info_t;

typedef struct _SENSOR_STATUS {
    uint16 impact_cnt;
    uint16 temperature;
}sensor_info_t;

static int16 calc_i2c_temperature(uint8 * i2c_data);

void init_batt_status_info(batt_info_t *p_battStatus);

int16 read_temperature();
//uint8 check_cable_status();

void sensor_status_init(sensor_info_t *p_sensor);

#endif
