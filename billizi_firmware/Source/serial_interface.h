#ifndef __SERIAL_INTERFACE__
#define __SERIAL_INTERFACE__

#include "OSAL.h"
#include "bcomdef.h"

#include "hal_uart.h"
#include "npi.h"

#include "gpio_interface.h"
#include "hw_mgr.h"
#include "flash_interface.h"

#define RX_BUFF_SIZE    20
#define INIT_LEN        8
#define BATT_INFO_LEN   12
#define BATT_LOG_LEN    17

#define HEADER_INFO     0x10
#define HEADER_LOG      0x20

#define PACKET_START    0x00
#define PACKET_END      0xFF

/** CALLBACKS
 * @fn      cb_rx_PacketParser
 * @brief   serial RxData parsing callback function
 */
void cb_rx_PacketParser( uint8 port, uint8 events );

/** DEBUG FUNCTINOS 
 * @fn      print_uart
 * @brief   Serial communication, transmit Tx Datas
 */
void print_uart(char *tx_data, ...);

void transmit_comm_data(uint8 tx_size, uint8 *tx_data);
void transmit_control_packet(uint8 packet_type);
void transmit_data_stream(uint8 tx_len, uint8 *tx_data);

uint8 *get_head_packet(Control_flag_t *apst_flags, batt_info_t *apst_BattStatus, uint16 log_cnt);
uint8 *get_log_packet(log_addr_t *apst_addr);

void uart_init(npiCBack_t npiCback);

#endif