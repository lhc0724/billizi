#include "log_interface.h"

/**
 * @fn write_log
 * @brief 받은 로그 패킷을 주소에 써넣는 함수
 * 
 * @param apst_log: 로그의 데이터가 들어있는 struct 포인터 변수
 * @param addr: 저장할 플래시의 주소
 */
uint8 write_log(log_data_t *apst_log, uint16 addr)
{
    uint32 ui_TmpData;

    ui_TmpData = ~(apst_log->word32);

    //tmpData에 로그값을 옮긴 후 포인터 변수에 key 값이 있다면 없애준다.
    //key 값은 한 로그의 집합당 맨 처음 하나만 기록
    if (apst_log->members.key) {
        apst_log->members.key = 0;
    }

    return write_flash(addr, &ui_TmpData);
}

/**
 * @fn read_log
 * @brief 주소를 받아 해당 주소의 로그를 읽어들이는 함수
 *        플래시는 0이 쓰기임으로 반전하여 리턴
 * 
 * @param addr: 읽어들일 플레시의 주소
 */
uint32 read_log(uint16 addr)
{
    uint32 ui_packet;
    
    read_flash(addr, FLOPT_UINT32, &ui_packet);
    
    return ~(ui_packet);
}

/**
 * @fn gen_newLogAddr
 * @brief 새로 기록할 로그시작 주소 발급
 * 
 * @return struct log_addr_t variable
 */
log_addr_t gen_newLogAddr()
{
    log_addr_t new_addr;
    
    uint32 ui_flashData;
    uint16 tmp = FLADDR_NEWLOG_ST;
    
    //search empty location of flash
    for(; tmp <= FLADDR_NEWLOG_ED; tmp++) {
        read_flash(tmp, FLOPT_UINT32, &ui_flashData);
        if(ui_flashData == EMPTY_FLASH) {
            break;
        }
    }

    if(tmp < FLADDR_NEWLOG_ED) {
        new_addr.key_addr = tmp;
        new_addr.offset_addr = tmp;
    }

    new_addr.log_cnt = 0;

    return new_addr;
}

/**
 * @fn init_new_log
 * @brief 새로운 로그 집합을 기록시 로그변수 초기화 함수
 */
log_data_t init_new_log()
{
    log_data_t st_newLog;
    
    st_newLog.word32 = 0;   //32bit 전체 초기화
    st_newLog.members.key = 1;      //새로 로그를 작성시 키값이 들어가도록 초기화

    return st_newLog;
}

/**
 * @fn get_last_log_range
 * @brief 최근에 기록된 로그의 범위를 찾아 해당 주소 리턴
 */
log_addr_t get_last_log_range()
{
    log_addr_t st_addr;
    uint32 ui_flashData;
    uint16 tmp_addr;

    tmp_addr = FLADDR_NEWLOG_ST;
    for (; tmp_addr <= FLADDR_NEWLOG_ED; tmp_addr++) {
        ui_flashData = read_log(tmp_addr);
        
        if(ui_flashData & MASK_LOG_KEY) {
            //update last key_addr
            st_addr.key_addr = tmp_addr;
        }else if(ui_flashData == 0) {   
            //empty flash
            break;
        }
    }

    //tmp_addr: 빈주소 혹은 가장 마지막 주소 + 1
    st_addr.log_cnt = tmp_addr - st_addr.key_addr;

    //해당 로그범위의 마지막 로그값을 가진 주소
    st_addr.offset_addr = tmp_addr - 1;

    return st_addr;
}

/**
 * @fn chk_valid_address
 * @brief 받은 주소가 로그를 저장 할 수 있는 주소인지 확인
 * @param addr: 검사할 주소
 * 
 * @return 0 or 1, 쓸 수 없는 주소 = 0 반환
 */
uint8 chk_valid_address(uint16 addr)
{
    if(addr < FLADDR_NEWLOG_ST || addr > FLADDR_NEWLOG_ED) {
        return 0;
    }
    return 1;
}
