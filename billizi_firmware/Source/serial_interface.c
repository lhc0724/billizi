#include "serial_interface.h"
#include "main_task.h"
#include "log_mgr.h"
#include <stdio.h>

static uint8 rx_buff[RX_BUFF_SIZE+1] = {0};
static uint8 rx_tail;

void cb_rx_PacketParser( uint8 port, uint8 events )
{
    (void)port; //unused input parameters
    uint8 num_bytes = Hal_UART_RxBufLen(NPI_UART_PORT);

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
                print_uart("ADC_OPEN-BATT\r\n");
                IO_ADC_INDUCTOR_SIDE = 0;
                IO_ADC_BATT_SIDE = 1;
                break;
            case 0x32:
                print_uart("ADC_OPEN-INDUT\r\n");
                IO_ADC_BATT_SIDE = 0;
                IO_ADC_INDUCTOR_SIDE = 1;
                break;
            case 0x33:
                print_uart("ADC_CLOSE\r\n");
                IO_ADC_BATT_SIDE = 0;
                IO_ADC_INDUCTOR_SIDE = 0;
                break;
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
        while(0 == NPI_WriteTransport(tx_data, tx_len + 1)) {}
    }
}

void transmit_control_packet(uint8 packet_type)
{
    uint8 tx_datas[8];

    if(packet_type) {
        //communication beginning.
        osal_memset(tx_datas, 0xFF, 8);
    }else {
        //end of packet.
        osal_memset(tx_datas, 0, 8);
    }

    transmit_comm_data(8, tx_datas);
}

void transmit_data_stream(uint8 tx_len, uint8 *tx_data) 
{
    transmit_control_packet(TRUE);

    transmit_comm_data(tx_len, tx_data);
    transmit_comm_data(tx_len, tx_data);

    transmit_control_packet(FALSE);
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

uint8 *get_head_packet(Control_flag_t *apst_flags, batt_info_t *apst_BattStatus, uint16 log_cnt)
{
    uint8 *comm_data;
    uint8 data_offset = 0;
    uint16 ui16_tmpdata;

    comm_data = osal_mem_alloc(sizeof(uint8) * 17);

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
        comm_data[data_offset++] = 0x01;
    } else {
        comm_data[data_offset++] = 0x02;
    }

    //current temperature data
    ui16_tmpdata = read_temperature();
    VOID osal_memcpy(comm_data + data_offset, (uint8*)&ui16_tmpdata, sizeof(uint16));   //14
    data_offset += sizeof(uint16);

    //number of log datas
    VOID osal_memcpy(comm_data + data_offset, (uint8*)&log_cnt, sizeof(uint16));        //16
    data_offset += sizeof(uint16);

    //packet length
    comm_data[data_offset] = data_offset;           //17

    return comm_data;
}   

uint8 *get_log_packet(log_addr_t *apst_addr) 
{
    uint8 *comm_data;
    uint8 data_offset = 0;
    uint8 *data_type;

    log_data_t batt_log;

    comm_data = osal_mem_alloc(sizeof(uint8) * 11);
    read_flash(apst_addr->offset_addr, FLOPT_UINT32, &batt_log.source_data);

    comm_data[data_offset++] = HEADER_LOG;  //1

    //log id
    apst_addr->log_cnt++;
    VOID osal_memcpy(comm_data+data_offset, (uint8*)&apst_addr->log_cnt, sizeof(uint16));   //3
    data_offset += sizeof(uint16);
    
    //log value type
    data_type = (uint8*)&batt_log.evt_info.event_datas;     //4
    comm_data[data_offset++] = (data_type[3] & 0x1F); 
    
    //log value(sensor, voltage, current, etc..)
    VOID osal_memcpy(comm_data+data_offset, (uint8*)batt_log.evt_info.log_value, sizeof(uint16));  //6
    data_offset += sizeof(uint16);

    //state machine information
    comm_data[data_offset++] = batt_log.evt_info.state; //7

    apst_addr->offset_addr++;
    if (apst_addr->offset_addr > FLADDR_LOGDATA_ED) {
        apst_addr->offset_addr = FLADDR_LOGDATA_ST;
    }

    //time stamp
    read_flash(apst_addr->offset_addr, FLOPT_UINT32, &batt_log.source_data);
    VOID osal_memcpy(comm_data + data_offset, (uint8*)batt_log.time_info.time_stamp, sizeof(uint8) * 3);  //10
    data_offset += 3;

    //packet length
    comm_data[data_offset] = data_offset;

    return comm_data;
}