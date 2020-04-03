#include "serial_interface.h"
#include "main_task.h"
#include "log_mgr.h"
#include <stdio.h>

static uint8 rx_buff[RX_BUFF_SIZE+1] = {0};
static uint8 rx_tail;
static uint16 debug_vals;

void cb_rx_PacketParser( uint8 port, uint8 events )
{
    (void)port; //unused input parameters
    uint8 num_bytes = Hal_UART_RxBufLen(NPI_UART_PORT);
    float tmp = 0;

    if(num_bytes) {
        //print_uart("VALID\r\n");
        if ((rx_tail + num_bytes) > RX_BUFF_SIZE) {
            osal_memset(rx_buff, 0, RX_BUFF_SIZE);
            rx_tail = 0;
        }
        HalUARTRead(NPI_UART_PORT, rx_buff + rx_tail, num_bytes);
        if(rx_buff[rx_tail] == 0x08) {
            rx_buff[rx_tail] = 0;
            if (rx_tail > 0) {
                rx_tail--;
            }
        }else {
            rx_tail += num_bytes;
        }
    }

    if(rx_buff[rx_tail-1] >= 0x0A && rx_buff[rx_tail-1] <= 0x0D) {
        switch(rx_buff[0]) {
            case 0x31:
                get_gparam_calib(&tmp);
                //print_uart("%04X\r\n", read_adc_sampling(10, READ_BATT_SIDE));
                print_uart("%.2f[V], ", read_voltage(READ_BATT_SIDE));
                print_uart("%.2f\r\n", tmp);
                break;
            case 0x32:
                print_uart("%.2f[V]\r\n", read_voltage(READ_EXT));
                // print_uart("CONN_EN-");
                // if (EN_CONN_RETR) {
                //     print_uart("0");
                //     EN_CONN_RETR = 0;
                // } else {
                //     print_uart("1");
                //     EN_CONN_RETR = 1;
                // }
                // print_uart("\r\n");
                // break;
            case 0x33:
                get_main_params(PARAM_EVT_VALS, &debug_vals);
                print_uart("%X\r\n", debug_vals);
                // print_uart("BRK_TEST_EN-");
                // if (RETR_TEST_EN) {
                //     print_uart("0");
                //     RETR_TEST_EN = 0;
                // } else {
                //     print_uart("1");
                //     RETR_TEST_EN = 1;
                // }
                // print_uart("\r\n");
                break;
            // case 0x34:
            //     print_uart("STATUS-");
            //     if (RETR_CABLE_STATUS) {
            //         print_uart("1");
            //     }else {
            //         print_uart("0");
            //     }
            //     print_uart("\r\n");
            //     break;
        }
        rx_buff[rx_tail] = '\n';

        transmit_comm_data(rx_tail, rx_buff);
        osal_memset(rx_buff, 0, RX_BUFF_SIZE);
        rx_tail = 0;
    }
}

void uart_init(npiCBack_t npiCback) 
{
    if (npiCback == NULL) {
        NPI_InitTransport(cb_rx_PacketParser);
    }else {
        NPI_InitTransport(npiCback);
    }
    osal_memset(rx_buff, 0, RX_BUFF_SIZE);
    rx_tail = 0;
}

void transmit_comm_data(uint8 tx_len, uint8 *tx_data)
{
    if(tx_len > 0) {
        while(0 == NPI_WriteTransport(tx_data, tx_len)) {}
    }
}

void transmit_control_packet(uint8 packet_type)
{
    uint8 tx_datas[8];

    if(packet_type) {
        //communication beginning.
        //osal_memset(tx_datas, 1<<7, 8);
        osal_memset(tx_datas, 0xff, 8);
    }else {
        //end of packet.
        osal_memset(tx_datas, 0, 8);
    }

    transmit_comm_data(8, tx_datas);
}

void transmit_data_stream(uint8 tx_len, uint8 *tx_data) 
{
    uint8 packet_size = 0;
    packet_size = tx_len;

    // transmit_control_packet(FALSE);
    // transmit_control_packet(FALSE);
    transmit_control_packet(FALSE);
    //delay_us(100);

    if(tx_len > 0) {
        while (0 == NPI_WriteTransport(tx_data, tx_len)) {}
        while (0 == NPI_WriteTransport(tx_data, tx_len)) {}
        while (0 == NPI_WriteTransport(&packet_size, 1)) {}
    }
    //transmit_comm_data(1, &packet_size);

    transmit_control_packet(TRUE);
}

void print_uart(char *tx_data, ...) 
{
    uint8 tx_len;
    char tx_buff[25];

    va_list va_tmp;
    va_start(va_tmp, tx_data);
    vsprintf(tx_buff, tx_data, va_tmp);
    va_end(va_tmp);

    tx_len = osal_strlen(tx_buff);
    tx_buff[tx_len + 1] = 0;

    if(tx_len > 0) {
        while(0 == NPI_WriteTransport((uint8*)tx_buff, tx_len + 1)) {}
    }
}

// void print_hex(uint8 *tx_buff, uint8 size)
// {
//     uint8 i;
    
//     for(i = 0; i < size; i++) {
//         print_uart("0x%02X ", tx_buff[i]);
//     }
//     print_uart("\r\n");
// }

uint8 *get_head_packet(Control_flag_t *apst_flags, batt_info_t *apst_BattStatus, uint16 log_cnt)
{
    uint8 *comm_data;
    uint8 data_offset = 0;
    uint16 ui16_tmpdata;

    comm_data = osal_mem_alloc(sizeof(uint8) * BATT_INFO_LEN);

    comm_data[data_offset++] = HEADER_INFO;         //1
    comm_data[data_offset++] = FIRMWARE_VERSION;    //2
    
    //get battery MAC address
    LL_ReadBDADDR(comm_data + data_offset);         //8
    data_offset += 6;

    //convert to uint16 types, (unit convert to V->mV)
    ui16_tmpdata = (uint16)(apst_BattStatus->batt_v * 1000);    
    VOID osal_memcpy(comm_data + data_offset, (uint8*)&ui16_tmpdata, sizeof(uint16));   //10
    data_offset += sizeof(uint16);

    //battery connector type
    comm_data[data_offset++] = load_flash_conntype();   //11

    //battery status data.
    if (apst_flags->abnormal & 0x1F) {                  //12
        comm_data[data_offset++] = 0x01;        //abnormal
    } else {
        comm_data[data_offset++] = 0x02;        //normaly
    }

    //current temperature data
    ui16_tmpdata = read_temperature();

    VOID osal_memcpy(comm_data + data_offset, (uint8*)&ui16_tmpdata, sizeof(uint16));   //14
    data_offset += sizeof(uint16);

    //number of log datas
    VOID osal_memcpy(comm_data + data_offset, (uint8*)&log_cnt, sizeof(uint16));        //16
    data_offset += sizeof(uint16);

    //packet length
    //comm_data[data_offset] = data_offset;           //17

    return comm_data;
}   

uint8 *get_log_packet(log_addr_t *apst_addr) 
{
    uint8 *comm_data;
    uint8 data_offset = 0;
    uint8 *tmp_data;

    log_data_t batt_log;
    time_data_t time_stamp;

    uint32 tmp_flash;

    comm_data = osal_mem_alloc(sizeof(uint8) * BATT_LOG_LEN);
    read_flash(apst_addr->offset_addr, FLOPT_UINT32, &tmp_flash);
    //print_uart("0x%04X, ", apst_addr->offset_addr);
    //print_uart("0x%08lX\r\n", tmp_flash);

    batt_log.data_all = tmp_flash;
    apst_addr->offset_addr = LOGADDR_VALIDATION(apst_addr->offset_addr + 1);

    comm_data[data_offset++] = HEADER_LOG;  //1

    //log id
    apst_addr->log_cnt++;
    VOID osal_memcpy(comm_data+data_offset, (uint8*)&apst_addr->log_cnt, sizeof(uint16));   //3
    data_offset += sizeof(uint16);
    
    //log type
    tmp_data = (uint8*)&batt_log.data_all;     //4
    comm_data[data_offset++] = batt_log.log_type;

    //data type: 어떤 데이터인지 알려줌 (전압, 전류, 충격, 온도)
    comm_data[data_offset++] = tmp_data[3];     //5

    //log value: 해당 데이터의 값(sensor, voltage, current, etc..)
    comm_data[data_offset++] = tmp_data[1];     //6
    comm_data[data_offset++] = tmp_data[2];     //7

    //state machine information
    comm_data[data_offset++] = batt_log.log_evt; //8

    //time stamp
    read_flash(apst_addr->offset_addr, FLOPT_UINT32, &time_stamp.data_all);
    apst_addr->offset_addr = LOGADDR_VALIDATION(apst_addr->offset_addr + 1);
    tmp_data = (uint8*)&time_stamp;
    if(tmp_data[0] == LOG_HEAD_TIME) {
        comm_data[data_offset++] = tmp_data[1];     //9
        comm_data[data_offset++] = tmp_data[2];     //10
        comm_data[data_offset++] = tmp_data[3];     //11
    }


    return comm_data;
}