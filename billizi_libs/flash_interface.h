#ifndef __FLASH_INTERFACE__
#define __FLASH_INTERFACE__

/* includes */
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"

#include "hci.h"
#include "gap.h"

#include "hal_flash.h"

/* Flash memory address values 
 * can use the available memory range is 0x8000 ~ 0xffff
 * flash can be erased by one page.
 * the size of a page is 512 block and one block is 4 byte.(512 * 4 = 2048 byte)
 * 0xN000 ~ 0xN1FF is a single page range.
 */
#define PG_END_OFFSET   0x01FF
#define EMPTY_FLASH     0xFFFFFFFF

#define ADDR_2_PAGE(address)    (address >> 9)
#define PAGE_2_ADDR(page)       (uint16)(page << 9)
#define ADDR_2_RECORD(address)  (address&PG_END_OFFSET)

/* flash read buffer type transfer */
#define _32BIT_2_16BIT(__VA__) (uint16*)(&__VA__)
#define _16BIT_2_32BIT(__VA__) *(uint32*)__VA__

/***********************
 * @common:
 * FLADDR_MIN: 사용가능한 메모리 영역의 최소 주소값
 * FLADDR_LOGKEY: 마지막 로그 기록 위치를 저장하는 메모리영역
 * FLADDR_LOGDATA: 로그 데이터가 저장되는 메모리 영역
 *      
 * Image A:
 * The available memory area is from 0x1000(8 page) to 0x8BFF(69 Page).
 * Image B:
 * The available memory area is from 0x8E00(71 page) to 0xF5FF(122 Page).
 */

#ifdef HAL_IMAGE_A
#define FLADDR_MIN             0x1000    //8 Page
#define FLADDR_CONNTYPE        0x1002
#define FLADDR_CALIB_REF       0x1003
#define FLADDR_CALIB_SELF_ST   0x1004
#define FLADDR_CALIB_SELF_ED   0x11FF

#define FLADDR_LOGKEY_ST    0x1200  //9 Page
#define FLADDR_LOGKEY_ED    0x13FF

#define FLADDR_LOGDATA_ST   0x1400  //10 Page
#define FLADDR_LOGDATA_ED   0x8BFF  //69 Page 0x7800

#define FLADDR_NEWLOG_ST   0x1200  //9 Page
#define FLADDR_NEWLOG_ED   0x8BFF  //69 Page

#else 
#define FLADDR_MIN        0x8E00    //71 Page
#define FLADDR_CONNTYPE        0x8E02
#define FLADDR_CALIB_REF       0x8E03
#define FLADDR_CALIB_SELF_ST   0x8E04
#define FLADDR_CALIB_SELF_ED   0x8FFF

#define FLADDR_LOGKEY_ST    0x9000  //72 Page
#define FLADDR_LOGKEY_ED    0x91FF

#define FLADDR_LOGDATA_ST   0x9200  //73 Page
#define FLADDR_LOGDATA_ED   0xF5FF  //122 Page 0x6400

#define FLADDR_NEWLOG_ST   0x9000  //72 Page
#define FLADDR_NEWLOG_ED   0xF5FF  //122 Page
#endif

/***********
 * log addres range over validation check 
 * this address range is over the max, return first log address.
 * and valid address, return input address.
 */
#define LOGADDR_VALIDATION(address) (address>FLADDR_LOGDATA_ED)?FLADDR_LOGDATA_ST:address
#define KEYADDR_VALIDATION(address) (address>FLADDR_LOGKEY_ED)?FLADDR_LOGKEY_ST:address
#define CALIBADDR_VALIDATION(address) (address>FLADDR_CALIB_SELF_ED)?FLADDR_CALIB_SELF_ST:address

typedef enum FLASH_VARIABLE_OPT {
    //Flash R/W variable type option
    FLOPT_UINT8,
    FLOPT_UINT16,
    FLOPT_UINT32,
    FLOPT_FLOAT
}eFlash_Var_t;

typedef enum _CONNECTOR_TYPE {
    CONN_MICRO_5   = 0x01,
    CONN_LIGHTNING = 0x02,
    CONN_USB_C     = 0x04
} eConnType_t;

typedef union _TIMES {
    struct {
        uint8 log_evt : 8;    
        uint32 time_value : 24; //timer unit: seconds
    };
    uint32 data_all;
}time_data_t;

typedef struct _FLASH_VARSIZE {
    union {
        struct {
            uint32 byte_1 : 8;
            uint32 byte_2 : 8;
            uint32 byte_3 : 8;
            uint32 byte_4 : 8;
        };
    }_8bits;

    union {
        struct {
            uint32 high_bit : 16;
            uint32 low_bit : 16;
        };
    }_16bits;

    uint32 _32bits;
}flash_types_t;

typedef union _FLASH_TYPE16 {
    struct {
        uint16 high_16bit : 16;
        uint16 low_16bit : 16;
    };
    uint32 all_bits;
}flash_16bit_t;

typedef union _FLASH_TYPE8 {
    struct {
        uint8 byte_1 : 8;
        uint8 byte_2 : 8;
        uint8 byte_3 : 8;
        uint8 byte_4 : 8;
    };
    uint32 all_bits;
}flash_8bit_t;

uint8 write_flash(uint16 ai_addr, void *p_value);
void read_flash(uint16 ai_addr, eFlash_Var_t value_type, void *p_value);

uint8 stored_conn_type(eConnType_t ai_connType);
uint16 stored_adc_calib(uint16 calib_ref);
uint8 load_flash_conntype();

void erase_flash_range(uint16 st_addr, uint16 end_addr);
void erase_flash_log_area();

void init_flash_mems(uint16 ai_addr);
void init_calib_mem_page();

uint16 get_calib_address();
uint16 search_self_calib();
void update_self_calibration(uint8 calib_status, uint16 adc_value);

#endif
