#include "flash_interface.h"

void read_flash(uint16 ai_addr, eFlash_Var_t value_type, void *p_value)
{
    uint8 tmp[4];

    HalFlashRead(ADDR_2_PAGE(ai_addr), ADDR_2_RECORD(ai_addr) * 4, tmp, HAL_FLASH_WORD_SIZE);
    switch(value_type) {
        case FLOPT_UINT8:
            *((uint8*)p_value) = *(uint8*)tmp;
            break;
        case FLOPT_UINT16:
            *((uint16*)p_value) = *(uint16*)tmp;
            break;
        case FLOPT_UINT32:
            *((uint32 *)p_value) = *(uint32*)tmp;
            break;
        case FLOPT_FLOAT:
            *((float *)p_value) = *(float*)tmp;
            break;
    }
}

uint8 write_flash(uint16 ai_addr, void *p_value)
{
    uint8 *tmp_value;
    uint32 validation;

    tmp_value = ((uint8 *)p_value);

    HalFlashWrite(ai_addr, tmp_value, 1);
    read_flash(ai_addr, FLOPT_UINT32, &validation);

    if (validation != *(uint32 *)tmp_value) {
        return 1;
    }
    return 0;
}

float load_flash_calib()
{
    float g_calib;
    read_flash(FLADDR_CALIB_REF, FLOPT_FLOAT, &g_calib);

    return g_calib;
}

uint8 stored_conn_type(eConnType_t ai_connType)
{
    flash_8bit_t conn_type;
    uint8 result = 1;
    
    conn_type.all_bits = 0;

    read_flash(FLADDR_CONNTYPE, FLOPT_UINT32, &conn_type);
    if (conn_type.byte_1 != 0xFF && conn_type.byte_1 <= 0x03) {
        return result;
    }

    switch(ai_connType) {
        case CONN_LIGHTING:
        case CONN_MICRO_5:
        case CONN_USB_C:
            conn_type.byte_1 = ai_connType;
            result = write_flash(FLADDR_CONNTYPE, &conn_type.all_bits);
            break;
        default:
            break;
    }

    return result;
}

uint16 stored_adc_calib(uint16 calib_ref)
{
    flash_16bit_t calib_datas;

    if(calib_ref == 0) {
        /*******
         * store to self-calibration option
         * set to calib_ref address value is 0.
         * this battery system performs self-calibration. */
        calib_datas.all_bits = 0;
        write_flash(FLADDR_CALIB_REF, &calib_datas.all_bits);

        /*******************
         * setup the initial calibration reference adc value
         * Maximum Voltage = 6885 * (1.25/8191) * 4 = 4.202784V
         */
        calib_datas.high_16bit = 0xFFFF;
        calib_datas.low_16bit = 6885;
        write_flash(FLADDR_CALIB_SELF_ST, &calib_datas.all_bits);
    } else {
        calib_datas.high_16bit = 0x1000;  //reference flag
        calib_datas.low_16bit = calib_ref;
        write_flash(FLADDR_CALIB_REF, &calib_datas.all_bits);
    }

    return calib_datas.low_16bit;
}

uint8 load_flash_conntype() 
{
    flash_8bit_t conn_type;

    read_flash(FLADDR_CONNTYPE, FLOPT_UINT32, &conn_type.all_bits);

    return conn_type.byte_1;
}

void init_flash_mems(uint16 ai_addr)
{
    uint8 pg;
    uint32 flash_val;

    pg = ADDR_2_PAGE(ai_addr);
    read_flash(ai_addr, FLOPT_UINT32, &flash_val);
    if(flash_val != EMPTY_FLASH) {
        HalFlashErase(pg);
    }

}

void erase_flash_range(uint16 st_addr, uint16 end_addr)
{
    uint8 offset_pg, end_pg;
    uint8 i;

    offset_pg = ADDR_2_PAGE(st_addr);
    end_pg = ADDR_2_PAGE(end_addr);

    for(i = offset_pg; i <= end_pg; i++) {
        HalFlashErase(i);
    }
}

void erase_flash_log_area()
{
    uint8 pg;
    pg = ADDR_2_PAGE(FLADDR_LOGKEY_ST);
    for(; pg <= ADDR_2_PAGE(FLADDR_LOGDATA_ED); pg++) {
        HalFlashErase(pg);
    }
}