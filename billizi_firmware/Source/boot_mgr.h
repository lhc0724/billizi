#ifndef __BOOT_MANAGER__
#define __BOOT_MANAGER__

#include "gpio_interface.h"
#include "adc_interface.h"
#include "flash_interface.h"
#include "serial_interface.h"
#include "hw_mgr.h"

#define BOOT                    0x0010

extern void Billizi_BootMgr_Init(uint8 task_id);
extern uint16 Billizi_BootMgr_ProcessEvent(uint8 task_id, uint16 events);

#endif