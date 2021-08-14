#ifndef __LOG_MANAGER__
#define __LOG_MANAGER__

#include "flash_interface.h"
#include "hw_mgr.h"

#define LOG_HEAD_NO_SERV 0x01
#define LOG_HEAD_EN_SERV 0x02
#define LOG_HEAD_DISCHG_ST 0x04
#define LOG_HEAD_DISCHG_ED 0x08
#define LOG_HEAD_ABNORMAL 0x10
#define LOG_HEAD_TIME 0x20

#define LOG_EVT_DISCHG_ST 0x01
#define LOG_EVT_DISCHG_ED 0x02
#define LOG_EVT_OVER_TEMP 0x04
#define LOG_EVT_OVER_CURR 0x08
#define LOG_EVT_IMPACT    0x10
#define LOG_EVT_BRK_CABLE 0x20
#define LOG_EVT_PWR_OFF   0x40

//#define TYPE_TIME_LOG   0x00  
#define TYPE_HEAD_LOG   0x01
#define TYPE_TAIL_LOG   0x02
#define TYPE_NORMAL_LOG 0x00

// #define TYPE_TIME_LOG   0x00  
// #define TYPE_HEAD_LOG   0x01
// #define TYPE_TAIL_LOG   0x02
// #define TYPE_NORMAL_LOG 0x04 
#define TYPE_INVALID_LOG (TYPE_TAIL_LOG | TYPE_HEAD_LOG | TYPE_NORMAL_LOG)

//flash_interface.h 있는 log_data_t를 대체할 스트럭쳐
//아직 사용하지 않는중..
typedef struct _NEW_LOGS {
    union {
        struct {
            uint16 log_type : 4;
            uint16 events : 8;
            uint16 values : 16;
        };
        uint32 package;
    } datas;

    uint32 time;

} st_newLog_t;

uint8 log_system_init(log_data_t *apst_log, log_addr_t *apst_addr);
uint8 LogAddress_valid_check(uint16 addr);

void generate_new_log_address(log_addr_t *apst_addr);

uint16 get_key_address();
void stroed_key_value(log_addr_t *apst_addr);

uint16 analysis_keylog(uint16 key_addr);
uint16 analysis_tail_log(uint16 key_value);
uint16 search_head_log(uint16 offset);

uint8 stored_log_data(log_addr_t *apst_addr, log_data_t *apst_data, time_data_t *apst_times);
uint16 wrtie_tail_log(uint16 ai_addr, Control_flag_t ast_flag);

uint16 calc_number_of_LogDatas(log_addr_t ast_addr);

#endif