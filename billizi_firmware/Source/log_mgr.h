#ifndef __LOG_MANAGER__
#define __LOG_MANAGER__

#include "flash_interface.h"
#include "hw_mgr.h"

#define TIME_HEAD   0x50

uint8 log_system_init(log_data_t *apst_log, log_addr_t *apst_addr);
uint8 LogAddress_valid_check(uint16 addr);

void generate_new_log_address(log_addr_t *apst_addr);

uint16 get_key_address();
uint16 log_location_parser(uint16 key_addr);
uint16 check_key_log(uint16 keylog_addr, Control_flag_t *apst_flags);
uint16 search_head_log(uint16 offset);
flash_16bit_t search_self_calib();

uint16 calc_number_of_LogDatas(log_addr_t *apst_addr);

#endif