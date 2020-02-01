#ifndef __LOG_MANAGER__
#define __LOG_MANAGER__

#include "flash_interface.h"
#include "hw_mgr.h"

#define LOG_HEAD_NO_SERV    0x01
#define LOG_HEAD_EN_SERV    0x02
#define LOG_HEAD_DISCHG     0x04
#define LOG_HEAD_ABNORMAL   0x08
#define LOG_HEAD_TIME       0x10

#define TYPE_NORMAL_LOG     0x00
#define TYPE_HEAD_LOG       0x01
#define TYPE_KEY_LOG        0x02
#define TYPE_INVALID_LOG (TYPE_KEY_LOG | TYPE_HEAD_LOG)

uint8 log_system_init(log_data_t *apst_log, log_addr_t *apst_addr);
uint8 LogAddress_valid_check(uint16 addr);

void generate_new_log_address(log_addr_t *apst_addr);

uint16 get_key_address();
uint16 log_location_parser(uint16 key_addr);
uint16 check_key_log(uint16 keylog_addr, Control_flag_t *apst_flags);
uint16 search_head_log(uint16 offset);
flash_16bit_t search_self_calib();

uint8 stored_log_data(log_addr_t *apst_addr, log_data_t *apst_data, time_data_t *apst_times);

uint16 calc_number_of_LogDatas(log_addr_t *apst_addr);

#endif