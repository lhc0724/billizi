#ifndef __LOG_INTERFACE__
#define __LOG_INTERFACE__

#include "flash_interface.h"

#define MASK_LOG_KEY    0x00000001
#define MASK_LOG_TYPE   0x00000006
#define MASK_LOG_STATE  0x0000FF00
#define MASK_LOG_VALUE  0xFFFF0000

typedef enum _LOG_TYPES {
	TYPE_VOLTAGE,   // 00: voltage
	TYPE_CURRENT,   // 01: current
	TYPE_TEMP,      // 10: temperature
	TYPE_VIB,       // 11: vibration sensor
} eLogType_t;

typedef struct _FLASH_ADDR {
	uint16 key_addr;    //log start point address
	uint16 offset_addr;  //log current offset
	uint16 log_cnt; //offset - key = log number
} log_addr_t;

typedef union _LOG_DATA {
	struct {
		uint8 key : 1;      //각 로그 집합의 시작을 뜻하는 비트
		uint8 type : 2;     //해당 로그 데이터 값의 항목
		uint8 hw_status : 5;    //아직 안정함
		uint8 state : 8;    //로그가 기록된 state 값
		uint16 value : 16;  //로그의 데이터값
	} members;
	uint32 word32;
} log_data_t;

uint8 write_log(log_data_t *apst_log, uint16 addr);
uint32 read_log(uint16 addr);

log_data_t init_new_log();
log_addr_t gen_newLogAddr();
log_addr_t get_last_log_range();

uint8 chk_valid_address(uint16 addr);

#endif // __LOG_INTERFACE__
