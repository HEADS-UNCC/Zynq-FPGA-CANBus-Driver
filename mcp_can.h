
#ifndef MCP_CAN_H_
#define MCP_CAN_H_

#include "mcp_can_dfs.h"
#define MAX_CHAR_IN_MESSAGE 8

typedef struct {
	u8 mcpMode;
	u8 m_nExtFlg;
	unsigned int m_nID;
	u8 m_nDlc;
	u8 m_nDta[MAX_CHAR_IN_MESSAGE];
	u8 m_nRtr;
	u8 m_nfilhit;
	u8 MCPCS;
} mcp_can_struct;

int InitCAN(XSpiPs * SpiInstancePtr, u16 SpiDeviceID);
void mcp2515_reset(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp2515_read_reg(XSpiPs * SpiInstancePtr, u8  SlaveSelect, u8 RegAddress);
void mcp2515_read_reg_n(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 * values, u8 length);
void mcp2515_set_reg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 Value);
void mcp2515_set_reg_n(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 * values, u8 length);
void mcp2515_modify_reg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 Mask, u8 value);
u8 mcp2515_read_status(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp2515_set_CANCTRL_mode(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 newmode);
u8 mcp2515_set_mode(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 opmode);
u8 mcp2515_config_rate(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 can_speed, u8 can_clock);
void mcp2515_write_mf(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 mcp_addr, u8 ext, unsigned int id);
void mcp2515_init_CAN_buffers(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp2515_init(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 canIDMode, u8 canSpeed, u8 canClock);
void mcp2515_write_id(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 mcp_addr, u8 ext, unsigned int id );
void mcp2515_read_id(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 mcp_addr, u8 * ext, unsigned int * id );
void mcp2515_write_can_msg(XSpiPs * SpiInstancePtr, u8 SlaveSelect,mcp_can_struct * mcp_can, u8 buffer_sidh_addr);
void mcp2515_read_can_msg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 buffer_sidh_addr);
u8 mcp2515_get_next_free_TX_buf(XSpiPs *SpiInstancePtr, u8 SlaveSelect,  u8 *txbuf_n);
u8 mcp_can_set_msg(mcp_can_struct * mcp_can, unsigned int id, u8 rtr, u8 ext, u8 len, u8 *pData);
u8 mcp_can_clear_msg(mcp_can_struct * mcp_can);
u8 mcp2515_read_msg_buf(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, unsigned int *id, u8 *ext, u8 *len, u8 buf[]);
u8 mcp2515_read_msg_buf_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, unsigned int *id, u8 *len, u8 buf[]);
u8 mcp2515_check_error(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8  mcp2515_get_error(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp2515_error_count_rx(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp2515_error_count_tx(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp_can_en_one_shot_tx(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp2515_dis_one_shot_tx(XSpiPs * SpiInstancePtr, u8 SlaveSelect);
u8 mcp2515_init_mask(XSpiPs * SpiInstancePtr, u8 SlaveSelect,mcp_can_struct * mcp_can, u8 num, u8 ext, unsigned int ulData);
u8 mcp2515_init_mask_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 num, unsigned int ulData);
u8 mcp2515_init_filt(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 num, u8 ext, unsigned int ulData);
u8 mcp2515_init_filt_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 num, unsigned int ulData);
u8 mcp2515_send_msg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can);
u8 mcp2515_send_msg_buf(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, unsigned int id, u8 ext, u8 len, u8 *buf);
u8 mcp2515_send_msg_buf_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect,mcp_can_struct * mcp_can,  unsigned int id, u8 len, u8 *buf);


#endif /* MCP_CAN_H_ */
