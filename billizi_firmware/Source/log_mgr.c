#include "log_mgr.h"

uint8 log_system_init(log_data_t *apst_log, log_addr_t *apst_addr) 
{
    //log data structure initialize
    apst_log->evt_info.event_datas = 0;
    apst_log->time_info.time_datas = 0;
    apst_log->time_info.time_header = TIME_HEAD;

    //log address information structure initialize
    apst_addr->key_addr = 0;
    apst_addr->head_addr = 0;
    apst_addr->offset_addr = 0;
    apst_addr->tail_addr = 0;
    apst_addr->log_cnt = 0;

    return 0;
}

uint8 LogAddress_valid_check(uint16 addr)
{
    if(addr < FLADDR_LOGDATA_ST || addr > FLADDR_LOGDATA_ED) {
        return FALSE;
    }
    return TRUE;
}

/**
 * @fn      get_key_address
 * @brief   search last stored key address
 */
uint16 get_key_address()
{
    uint16 key_addr = FLADDR_LOGKEY_ED;
    flash_16bit_t key_value;

    /* search last valid key location */
    while (key_addr >= FLADDR_LOGKEY_ST) {
        read_flash(key_addr, FLOPT_UINT32, &key_value.all_bits);
        if (key_value.all_bits != EMPTY_FLASH) {
            break;
        }
        key_addr--;
    }

    // /* key address memory check */
    // if(key_value.high_16bit == 0xFFFF) {
    //     return key_addr;
    // }else {
    //     key_addr++;
    //     /* if key address is over, key stored page erase and return first address of this page */
    //     if(key_addr > FLADDR_LOGKEY_ED) {
    //         HalFlashErase(ADDR_2_PAGE(FLADDR_LOGKEY_ST));
    //         key_addr = FLADDR_LOGKEY_ST;
    //     }
    // }

    return key_addr;
}

uint16 log_location_parser(uint16 key_addr)
{
    flash_16bit_t key_value;

    read_flash(key_addr, FLOPT_UINT32, &key_value.all_bits);

    if(key_value.all_bits == EMPTY_FLASH) {
        return 0;
    }
    
    if(key_value.high_16bit != 0xFFFF) {
        return key_value.high_16bit;
    }else {
        return key_value.low_16bit;
    }
}

uint16 check_key_log(uint16 keylog_addr, Control_flag_t *apst_flags)
{
    log_data_t tmp_log;

    //read keylog data
    read_flash(keylog_addr, FLOPT_UINT32, &tmp_log.evt_info);

    if(!tmp_log.evt_info.key_flag) {
        //key flag match error, this log is invalid.
        apst_flags->abnormal = 1;
        apst_flags->need_comm = 1;
        return search_head_log(keylog_addr);
    }

    if(tmp_log.evt_info.state == 0x08) {
        //0x08 is abnormal state
        apst_flags->abnormal = 1;
    }

    if(tmp_log.evt_info.clc_flag) {
        /* clc flag is true, before not completed communication with kiosk. */
        apst_flags->need_comm = 1;
        return tmp_log.evt_info.log_value;
    }

    //the key log status is normal.
    return 0;
}

uint16 search_head_log(uint16 offset)
{
    log_data_t tmp_log;
    uint16 tmp_addr;

    if(offset > FLADDR_LOGDATA_ST) {
        tmp_addr = offset - 1;
    }else {
        tmp_addr = FLADDR_LOGDATA_ED;
    }

    while (tmp_addr != offset) {
        read_flash(tmp_addr, FLOPT_UINT32, &tmp_log.evt_info);
        if(tmp_log.evt_info.head_flag) {
            break;
        }else {
            tmp_addr--;
            if(tmp_addr < FLADDR_LOGDATA_ST) {
                tmp_addr = FLADDR_LOGDATA_ED;
            }
        }
    }
    
    if(tmp_addr == offset) {
        //not found head log address
        return 0xFFFF;
    }

    return tmp_addr;
}

flash_16bit_t search_self_calib()
{
    flash_16bit_t flash_value;
    uint16 i;
    for(i = FLADDR_CALIB_SELF_ED; i >= FLADDR_CALIB_SELF_ST; i--) {
        read_flash(i, FLOPT_UINT32, &flash_value.all_bits);
        if (flash_value.all_bits != EMPTY_FLASH) { break; }
    }

    if (i < FLADDR_CALIB_SELF_ST) {
        //not found calibration value
        flash_value.all_bits = EMPTY_FLASH;
    }

    return flash_value;   
}

uint16 calc_number_of_LogDatas(log_addr_t *apst_addr)
{
    uint16 log_cnt;

    if (apst_addr->head_addr > apst_addr->tail_addr) {
        //Log address after ring buffer rotation
        log_cnt = FLADDR_LOGDATA_ED - apst_addr->head_addr;
        log_cnt += apst_addr->tail_addr - FLADDR_LOGDATA_ST;
    } else if (apst_addr->tail_addr > apst_addr->head_addr) {
        log_cnt = apst_addr->tail_addr - apst_addr->head_addr;
    }

    return log_cnt/8;
}