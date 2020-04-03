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
        case CONN_LIGHTNING:
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

void init_calib_mem_page()
{
    uint32 conn_type = 0;
    uint32 calib_ref = 0;
    read_flash(FLADDR_CONNTYPE, FLOPT_UINT32, &conn_type);
    read_flash(FLADDR_CALIB_REF, FLOPT_UINT32, &calib_ref);

    HalFlashErase(ADDR_2_PAGE(FLADDR_CALIB_SELF_ST));
    write_flash(FLADDR_CONNTYPE, &conn_type);
    write_flash(FLADDR_CONNTYPE, &calib_ref);
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

uint16 get_calib_address()
{
    flash_16bit_t flash_value;
    uint16 i;
    for (i = FLADDR_CALIB_SELF_ED; i >= FLADDR_CALIB_SELF_ST; i--) {
        read_flash(i, FLOPT_UINT32, &flash_value.all_bits);
        if (flash_value.all_bits != EMPTY_FLASH) { break; }
    }

     if (i < FLADDR_CALIB_SELF_ST) {
        //not found calibration value
        return 0;
    }

    return i;
}

uint16 search_self_calib()
{
    flash_16bit_t flash_value;
    uint16 calib_addr = get_calib_address();

    if (!calib_addr) {
        return 0;
    }
    read_flash(calib_addr, FLOPT_UINT32, &flash_value.all_bits);
    
    if(flash_value.high_16bit != 0xFFFF) {
        return flash_value.high_16bit;
    }

    return flash_value.low_16bit;
}

void update_self_calibration(uint8 calib_status, uint16 adc_value)
{
    flash_16bit_t calib_datas;
    uint16 calib_addr;
    
    //check current calibration value type.
    //0: reference 1: self
    if(!calib_status) {
        //delete reference calibration value
        calib_datas.all_bits = 0;
        write_flash(FLADDR_CALIB_REF, &calib_datas.all_bits);
    }

    calib_addr = get_calib_address();
    if (!calib_addr) {
        calib_addr = FLADDR_CALIB_SELF_ST;
    }

    read_flash(calib_addr, FLOPT_UINT32, &calib_datas.all_bits);

    if (calib_datas.high_16bit != 0xFFFF) {
        //보정값을 저장할 해당 메모리주소의 잔여공간이 없을경우
        calib_addr = CALIBADDR_VALIDATION(calib_addr + 1);
        if (calib_addr == FLADDR_CALIB_SELF_ST) {
            init_calib_mem_page();
        }
        calib_datas.high_16bit = 0xFFFF;
        calib_datas.low_16bit = adc_value;
    }else {
        calib_datas.high_16bit = adc_value;
    }

    write_flash(calib_addr, &calib_datas.all_bits);
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
    pg = ADDR_2_PAGE(FLADDR_CALIB_REF);
    for(; pg <= ADDR_2_PAGE(FLADDR_LOGDATA_ED); pg++) {
        HalFlashErase(pg);
    }
}