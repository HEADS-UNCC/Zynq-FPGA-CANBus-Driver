
#include <stdlib.h>
#include "mcp_can.h"
#include "sleep.h"
int InitCAN(XSpiPs * SpiInstancePtr, u16 SpiDeviceID){
	XSpiPs_Config *SpiConfig;
	int status =-1;
	SpiConfig = XSpiPs_LookupConfig(SpiDeviceID);
	if (SpiConfig == NULL){
		xil_printf("Error looking up device config.\r\n");
		return XST_FAILURE;
	}
	status = XSpiPs_CfgInitialize(SpiInstancePtr, SpiConfig, SpiConfig->BaseAddress);
	if (status!= XST_SUCCESS){
		xil_printf("Error Initializing configuration.\r\n");
		return XST_FAILURE;
	}

	status = XSpiPs_SelfTest(SpiInstancePtr);
	if (status != XST_SUCCESS){
		xil_printf("Error performing self test\r\n");
		return XST_FAILURE;
	}
	XSpiPs_SetOptions(SpiInstancePtr, XSPIPS_MASTER_OPTION | XSPIPS_FORCE_SSELECT_OPTION);
	XSpiPs_SetClkPrescaler(SpiInstancePtr, XSPIPS_CLK_PRESCALE_16);
    XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xF);
	return XST_SUCCESS;
}

void mcp2515_reset(XSpiPs * SpiInstancePtr, u8 SlaveSelect){
	u8 buffer = MCP_RESET;
	XSpiPs_SetSlaveSelect(SpiInstancePtr, SlaveSelect);
    XSpiPs_PolledTransfer(SpiInstancePtr, &buffer, NULL, 1);
    XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xF); //Deselect all slaves.
    usleep(10000);
}

u8 mcp2515_read_reg(XSpiPs * SpiInstancePtr, u8  SlaveSelect, u8 RegAddress){
	u8 buffer[5];
	memset(buffer, 0x0, 4 * sizeof(u8));
	buffer[0] = MCP_READ;
	buffer[1] = RegAddress;
	XSpiPs_SetSlaveSelect(SpiInstancePtr, SlaveSelect);
	XSpiPs_PolledTransfer(SpiInstancePtr, buffer, buffer+2, 3);
	XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xf);
	return buffer[4];
}

void mcp2515_read_reg_n(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 * values, u8 length) {
	u8 buffer[32];
	//u8 *buffer = (u8 *)malloc((length+3) * sizeof(u8));
	memset(buffer, 0, (length +4) * sizeof(u8));

	buffer[0] = MCP_READ;
	buffer[1] = RegAddress;
	XSpiPs_SetSlaveSelect(SpiInstancePtr, SlaveSelect);
	XSpiPs_PolledTransfer(SpiInstancePtr, buffer, buffer+2, length+2);
	XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xf);
	memcpy(values, buffer+4, length * sizeof(u8));
}

void mcp2515_set_reg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 Value){
	u8 buffer[3];
	buffer[0] = MCP_WRITE;
	buffer[1] = RegAddress;
	buffer[2] = Value;
    XSpiPs_SetSlaveSelect(SpiInstancePtr, SlaveSelect);
    XSpiPs_PolledTransfer(SpiInstancePtr, buffer, NULL, 3);
    XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xF); //Deselect all slaves.
    usleep(250);
}

void mcp2515_set_reg_n(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 * values, u8 length) {
	u8 buffer[32];
//	u8 *buffer = (u8 *)malloc((length+2) * sizeof(u8));
	buffer[0] = MCP_WRITE;
	buffer[1] = RegAddress;
	memcpy(buffer+2, values, length * sizeof(u8));

	XSpiPs_SetSlaveSelect(SpiInstancePtr, SlaveSelect);
	XSpiPs_PolledTransfer(SpiInstancePtr, buffer, NULL, length+2);
	XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xf);
	usleep(250);
}
void mcp2515_modify_reg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 RegAddress, u8 Mask, u8 value){
	u8 buffer[4];
	buffer[0] = MCP_BITMOD;
	buffer[1] = RegAddress;
	buffer[2] = Mask;
	buffer[3] = value;
	XSpiPs_SetSlaveSelect(SpiInstancePtr, SlaveSelect);
	XSpiPs_PolledTransfer(SpiInstancePtr, buffer, NULL, 4);
	XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xf);
	usleep(250);
}
u8 mcp2515_read_status(XSpiPs * SpiInstancePtr, u8 SlaveSelect){
	u8 buffer[3];
	buffer[0] = MCP_READ_STATUS;
	XSpiPs_SetSlaveSelect(SpiInstancePtr, SlaveSelect);
	XSpiPs_PolledTransfer(SpiInstancePtr, buffer, buffer+1, 3);
	XSpiPs_SetSlaveSelect(SpiInstancePtr, 0xf);
	return buffer[2];
}
u8 mcp2515_set_CANCTRL_mode(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 newmode){
	u8 i;
	mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_CANCTRL, MODE_MASK, newmode);
	i = mcp2515_read_reg(SpiInstancePtr, SlaveSelect, MCP_CANCTRL);
	i &= MODE_MASK;
	if (i==newmode)
		return MCP2515_OK;
	return MCP2515_FAIL;
}

u8 mcp2515_set_mode(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 opmode){
	mcp_can->mcpMode = opmode;
	return mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, opmode);
}

u8 mcp2515_config_rate(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 can_speed, u8 can_clock){
	u8 set, cfg1, cfg2, cfg3;
	set = (u8)(0x1);
	switch (can_clock)
	{
	case (MCP_8MHZ):
			switch (can_speed)
			{
			case (CAN_5KBPS):                                               //   5KBPS
				cfg1 = MCP_8MHz_5kBPS_CFG1;
			cfg2 = MCP_8MHz_5kBPS_CFG2;
			cfg3 = MCP_8MHz_5kBPS_CFG3;
			break;

			case (CAN_10KBPS):                                              //  10KBPS
				cfg1 = MCP_8MHz_10kBPS_CFG1;
			cfg2 = MCP_8MHz_10kBPS_CFG2;
			cfg3 = MCP_8MHz_10kBPS_CFG3;
			break;

			case (CAN_20KBPS):                                              //  20KBPS
				cfg1 = MCP_8MHz_20kBPS_CFG1;
			cfg2 = MCP_8MHz_20kBPS_CFG2;
			cfg3 = MCP_8MHz_20kBPS_CFG3;
			break;

			case (CAN_31K25BPS):                                            //  31.25KBPS
				cfg1 = MCP_8MHz_31k25BPS_CFG1;
			cfg2 = MCP_8MHz_31k25BPS_CFG2;
			cfg3 = MCP_8MHz_31k25BPS_CFG3;
			break;

			case (CAN_33K3BPS):                                             //  33.33KBPS
				cfg1 = MCP_8MHz_33k3BPS_CFG1;
			cfg2 = MCP_8MHz_33k3BPS_CFG2;
			cfg3 = MCP_8MHz_33k3BPS_CFG3;
			break;

			case (CAN_40KBPS):                                              //  40Kbps
				cfg1 = MCP_8MHz_40kBPS_CFG1;
			cfg2 = MCP_8MHz_40kBPS_CFG2;
			cfg3 = MCP_8MHz_40kBPS_CFG3;
			break;

			case (CAN_50KBPS):                                              //  50Kbps
				cfg1 = MCP_8MHz_50kBPS_CFG1;
			cfg2 = MCP_8MHz_50kBPS_CFG2;
			cfg3 = MCP_8MHz_50kBPS_CFG3;
			break;

			case (CAN_80KBPS):                                              //  80Kbps
				cfg1 = MCP_8MHz_80kBPS_CFG1;
			cfg2 = MCP_8MHz_80kBPS_CFG2;
			cfg3 = MCP_8MHz_80kBPS_CFG3;
			break;

			case (CAN_100KBPS):                                             // 100Kbps
				cfg1 = MCP_8MHz_100kBPS_CFG1;
			cfg2 = MCP_8MHz_100kBPS_CFG2;
			cfg3 = MCP_8MHz_100kBPS_CFG3;
			break;

			case (CAN_125KBPS):                                             // 125Kbps
				cfg1 = MCP_8MHz_125kBPS_CFG1;
			cfg2 = MCP_8MHz_125kBPS_CFG2;
			cfg3 = MCP_8MHz_125kBPS_CFG3;
			break;

			case (CAN_200KBPS):                                             // 200Kbps
				cfg1 = MCP_8MHz_200kBPS_CFG1;
			cfg2 = MCP_8MHz_200kBPS_CFG2;
			cfg3 = MCP_8MHz_200kBPS_CFG3;
			break;

			case (CAN_250KBPS):                                             // 250Kbps
				cfg1 = MCP_8MHz_250kBPS_CFG1;
			cfg2 = MCP_8MHz_250kBPS_CFG2;
			cfg3 = MCP_8MHz_250kBPS_CFG3;
			break;

			case (CAN_500KBPS):                                             // 500Kbps
				cfg1 = MCP_8MHz_500kBPS_CFG1;
			cfg2 = MCP_8MHz_500kBPS_CFG2;
			cfg3 = MCP_8MHz_500kBPS_CFG3;
			break;

			case (CAN_1000KBPS):                                            //   1Mbps
				cfg1 = MCP_8MHz_1000kBPS_CFG1;
			cfg2 = MCP_8MHz_1000kBPS_CFG2;
			cfg3 = MCP_8MHz_1000kBPS_CFG3;
			break;

			default:
				set = 0;
				return MCP2515_FAIL;
				break;
			}
			break;

			case (MCP_16MHZ):
			switch (can_speed)
			{
			case (CAN_5KBPS):                                               //   5Kbps
				cfg1 = MCP_16MHz_5kBPS_CFG1;
			cfg2 = MCP_16MHz_5kBPS_CFG2;
			cfg3 = MCP_16MHz_5kBPS_CFG3;
			break;

			case (CAN_10KBPS):                                              //  10Kbps
				cfg1 = MCP_16MHz_10kBPS_CFG1;
			cfg2 = MCP_16MHz_10kBPS_CFG2;
			cfg3 = MCP_16MHz_10kBPS_CFG3;
			break;

			case (CAN_20KBPS):                                              //  20Kbps
				cfg1 = MCP_16MHz_20kBPS_CFG1;
			cfg2 = MCP_16MHz_20kBPS_CFG2;
			cfg3 = MCP_16MHz_20kBPS_CFG3;
			break;

			case (CAN_33K3BPS):                                              //  20Kbps
				cfg1 = MCP_16MHz_33k3BPS_CFG1;
			cfg2 = MCP_16MHz_33k3BPS_CFG2;
			cfg3 = MCP_16MHz_33k3BPS_CFG3;
			break;

			case (CAN_40KBPS):                                              //  40Kbps
				cfg1 = MCP_16MHz_40kBPS_CFG1;
			cfg2 = MCP_16MHz_40kBPS_CFG2;
			cfg3 = MCP_16MHz_40kBPS_CFG3;
			break;

			case (CAN_50KBPS):                                              //  50Kbps
				cfg2 = MCP_16MHz_50kBPS_CFG2;
			cfg3 = MCP_16MHz_50kBPS_CFG3;
			break;

			case (CAN_80KBPS):                                              //  80Kbps
				cfg1 = MCP_16MHz_80kBPS_CFG1;
			cfg2 = MCP_16MHz_80kBPS_CFG2;
			cfg3 = MCP_16MHz_80kBPS_CFG3;
			break;

			case (CAN_100KBPS):                                             // 100Kbps
				cfg1 = MCP_16MHz_100kBPS_CFG1;
			cfg2 = MCP_16MHz_100kBPS_CFG2;
			cfg3 = MCP_16MHz_100kBPS_CFG3;
			break;

			case (CAN_125KBPS):                                             // 125Kbps
				cfg1 = MCP_16MHz_125kBPS_CFG1;
			cfg2 = MCP_16MHz_125kBPS_CFG2;
			cfg3 = MCP_16MHz_125kBPS_CFG3;
			break;

			case (CAN_200KBPS):                                             // 200Kbps
				cfg1 = MCP_16MHz_200kBPS_CFG1;
			cfg2 = MCP_16MHz_200kBPS_CFG2;
			cfg3 = MCP_16MHz_200kBPS_CFG3;
			break;

			case (CAN_250KBPS):                                             // 250Kbps
				cfg1 = MCP_16MHz_250kBPS_CFG1;
			cfg2 = MCP_16MHz_250kBPS_CFG2;
			cfg3 = MCP_16MHz_250kBPS_CFG3;
			break;

			case (CAN_500KBPS):                                             // 500Kbps
				cfg1 = MCP_16MHz_500kBPS_CFG1;
			cfg2 = MCP_16MHz_500kBPS_CFG2;
			cfg3 = MCP_16MHz_500kBPS_CFG3;
			break;

			case (CAN_1000KBPS):                                            //   1Mbps
				cfg1 = MCP_16MHz_1000kBPS_CFG1;
			cfg2 = MCP_16MHz_1000kBPS_CFG2;
			cfg3 = MCP_16MHz_1000kBPS_CFG3;
			break;

			default:
				set = 0;
				return MCP2515_FAIL;
				break;
			}
			break;

			case (MCP_20MHZ):
			switch (can_speed)
			{
			case (CAN_40KBPS):                                              //  40Kbps
				cfg1 = MCP_20MHz_40kBPS_CFG1;
			cfg2 = MCP_20MHz_40kBPS_CFG2;
			cfg3 = MCP_20MHz_40kBPS_CFG3;
			break;

			case (CAN_50KBPS):                                              //  50Kbps
				cfg1 = MCP_20MHz_50kBPS_CFG1;
			cfg2 = MCP_20MHz_50kBPS_CFG2;
			cfg3 = MCP_20MHz_50kBPS_CFG3;
			break;

			case (CAN_80KBPS):                                              //  80Kbps
				cfg1 = MCP_20MHz_80kBPS_CFG1;
			cfg2 = MCP_20MHz_80kBPS_CFG2;
			cfg3 = MCP_20MHz_80kBPS_CFG3;
			break;

			case (CAN_100KBPS):                                             // 100Kbps
				cfg1 = MCP_20MHz_100kBPS_CFG1;
			cfg2 = MCP_20MHz_100kBPS_CFG2;
			cfg3 = MCP_20MHz_100kBPS_CFG3;
			break;

			case (CAN_125KBPS):                                             // 125Kbps
				cfg1 = MCP_20MHz_125kBPS_CFG1;
			cfg2 = MCP_20MHz_125kBPS_CFG2;
			cfg3 = MCP_20MHz_125kBPS_CFG3;
			break;

			case (CAN_200KBPS):                                             // 200Kbps
				cfg1 = MCP_20MHz_200kBPS_CFG1;
			cfg2 = MCP_20MHz_200kBPS_CFG2;
			cfg3 = MCP_20MHz_200kBPS_CFG3;
			break;

			case (CAN_250KBPS):                                             // 250Kbps
				cfg1 = MCP_20MHz_250kBPS_CFG1;
			cfg2 = MCP_20MHz_250kBPS_CFG2;
			cfg3 = MCP_20MHz_250kBPS_CFG3;
			break;

			case (CAN_500KBPS):                                             // 500Kbps
				cfg1 = MCP_20MHz_500kBPS_CFG1;
			cfg2 = MCP_20MHz_500kBPS_CFG2;
			cfg3 = MCP_20MHz_500kBPS_CFG3;
			break;

			case (CAN_1000KBPS):                                            //   1Mbps
				cfg1 = MCP_20MHz_1000kBPS_CFG1;
			cfg2 = MCP_20MHz_1000kBPS_CFG2;
			cfg3 = MCP_20MHz_1000kBPS_CFG3;
			break;

			default:
				set = 0;
				return MCP2515_FAIL;
				break;
			}
			break;

			default:
				set = 0;
				return MCP2515_FAIL;
				break;
	}
	if (set) {
		mcp2515_set_reg(SpiInstancePtr, SlaveSelect, MCP_CNF1, cfg1);
		mcp2515_set_reg(SpiInstancePtr, SlaveSelect, MCP_CNF2, cfg2);
		mcp2515_set_reg(SpiInstancePtr, SlaveSelect, MCP_CNF3, cfg3);
		return MCP2515_OK;
	}
	return MCP2515_FAIL;
}

void mcp2515_write_mf(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 mcp_addr, u8 ext, unsigned int id){
	uint16_t canid;
	u8 tbufdata[4];
	canid = (uint16_t)(id & 0x0FFF);
	if ( ext == 1)
	{
		tbufdata[MCP_EID0] = (u8) (canid & 0xFF);
		tbufdata[MCP_EID8] = (u8) (canid >> 8);
		canid = (uint16_t)(id >> 16);
		tbufdata[MCP_SIDL] = (u8) (canid & 0x03);
		tbufdata[MCP_SIDL] += (u8) ((canid & 0x1C) << 3);
		tbufdata[MCP_SIDL] |= MCP_TXB_EXIDE_M;
		tbufdata[MCP_SIDH] = (u8) (canid >> 5 );
	}
	else
	{
		tbufdata[MCP_EID0] = (u8) (canid & 0xFF);
		tbufdata[MCP_EID8] = (u8) (canid >> 8);
		canid = (uint16_t)(id >> 16);
		tbufdata[MCP_SIDL] = (u8) ((canid & 0x07) << 5);
		tbufdata[MCP_SIDH] = (u8) (canid >> 3 );
	}

	mcp2515_set_reg_n( SpiInstancePtr, SlaveSelect, mcp_addr, tbufdata, 4 );
}

void mcp2515_init_CAN_buffers(XSpiPs * SpiInstancePtr, u8 SlaveSelect){
	u8 i, a1, a2, a3;
	u8 std = 0;
	u8 ext = 1;

	unsigned int ulMask = 0x00, ulFilt = 0x00;

    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXM0SIDH, ext, ulMask);			/*Set both masks to 0           */
    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXM1SIDH, ext, ulMask);			/*Mask register ignores ext bit */

                                                                        /* Set all filters to 0         */
    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF0SIDH, ext, ulFilt);			/* RXB0: extended               */
    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF1SIDH, std, ulFilt);			/* RXB1: standard               */
    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF2SIDH, ext, ulFilt);			/* RXB2: extended               */
    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF3SIDH, std, ulFilt);			/* RXB3: standard               */
    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF4SIDH, ext, ulFilt);
    mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF5SIDH, std, ulFilt);

    /* Clear, deactivate the three  */
    /* transmit buffers             */
    /* TXBnCTRL -> TXBnD7           */
    a1 = MCP_TXB0CTRL;
    a2 = MCP_TXB1CTRL;
    a3 = MCP_TXB2CTRL;
    for (i = 0; i < 14; i++) {                                          /* in-buffer loop               */
        mcp2515_set_reg(SpiInstancePtr, SlaveSelect, a1, 0);
        mcp2515_set_reg(SpiInstancePtr, SlaveSelect, a2, 0);
        mcp2515_set_reg(SpiInstancePtr, SlaveSelect, a3, 0);
        a1++;
        a2++;
        a3++;
    }
    mcp2515_set_reg(SpiInstancePtr, SlaveSelect, MCP_RXB0CTRL, 0);
    mcp2515_set_reg(SpiInstancePtr, SlaveSelect, MCP_RXB1CTRL, 0);
}

u8 mcp2515_init(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 canIDMode, u8 canSpeed, u8 canClock){
	u8 res;
	mcp2515_reset(SpiInstancePtr, SlaveSelect);
	mcp_can->mcpMode = MCP_LOOPBACK;
	res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, MODE_CONFIG);
	if (res>0){
		xil_printf("Entering Configuration Mode Failure...\r\n");
		return res;
	}
	xil_printf("Entering Configuration Mode Successful!\r\n");
	if (mcp2515_config_rate(SpiInstancePtr, SlaveSelect, canSpeed, canClock)){
		xil_printf("Setting Baudrate Failure...\r\n");
		return res;
	}
	xil_printf("Setting Baudrate Successful!\r\n");
	if (res == MCP2515_OK){
		mcp2515_init_CAN_buffers(SpiInstancePtr, SlaveSelect);
		mcp2515_set_reg(SpiInstancePtr, SlaveSelect, MCP_CANINTE, MCP_RX0IF | MCP_RX1IF);
        switch(canIDMode)
        {
            case (MCP_ANY):
				mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_RXB0CTRL,
				MCP_RXB_RX_MASK | MCP_RXB_BUKT_MASK,
				MCP_RXB_RX_ANY | MCP_RXB_BUKT_MASK);
				mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_RXB1CTRL, MCP_RXB_RX_MASK,
				MCP_RXB_RX_ANY);
            break;
            case (MCP_STDEXT):
				mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_RXB0CTRL,
				MCP_RXB_RX_MASK | MCP_RXB_BUKT_MASK,
				MCP_RXB_RX_STDEXT | MCP_RXB_BUKT_MASK );
				mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_RXB1CTRL, MCP_RXB_RX_MASK,
				MCP_RXB_RX_STDEXT);
            break;

            default:
            xil_printf("`Setting ID Mode Failure...\r\n");
            return MCP2515_FAIL;
            break;
        }
        res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, mcp_can->mcpMode);
        if (res){
        	xil_printf("Returning to Previous Mode Failure...\r\n");
        	return res;
        }
	}
	return res;
}

void mcp2515_write_id(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 mcp_addr, u8 ext, unsigned int id ){
    uint16_t canid;
    u8 tbufdata[4];

    canid = (uint16_t)(id & 0x0FFFF);

    if ( ext == 1)
    {
        tbufdata[MCP_EID0] = (u8) (canid & 0xFF);
        tbufdata[MCP_EID8] = (u8) (canid >> 8);
        canid = (uint16_t)(id >> 16);
        tbufdata[MCP_SIDL] = (u8) (canid & 0x03);
        tbufdata[MCP_SIDL] += (u8) ((canid & 0x1C) << 3);
        tbufdata[MCP_SIDL] |= MCP_TXB_EXIDE_M;
        tbufdata[MCP_SIDH] = (u8) (canid >> 5 );
    }
    else
    {
        tbufdata[MCP_SIDH] = (u8) (canid >> 3 );
        tbufdata[MCP_SIDL] = (u8) ((canid & 0x07 ) << 5);
        tbufdata[MCP_EID0] = 0;
        tbufdata[MCP_EID8] = 0;
    }

    mcp2515_set_reg_n(SpiInstancePtr, SlaveSelect, mcp_addr, tbufdata, 4 );
}

void mcp2515_read_id(XSpiPs * SpiInstancePtr, u8 SlaveSelect, u8 mcp_addr, u8 * ext, unsigned int * id ){
    u8 tbufdata[4];

    *ext = 0;
    *id = 0;

    mcp2515_read_reg_n(SpiInstancePtr, SlaveSelect, mcp_addr, tbufdata, 4 );

    *id = (tbufdata[MCP_SIDH]<<3) + (tbufdata[MCP_SIDL]>>5);

    if ( (tbufdata[MCP_SIDL] & MCP_TXB_EXIDE_M) ==  MCP_TXB_EXIDE_M )
    {
                                                                        /* extended id                  */
        *id = (*id<<2) + (tbufdata[MCP_SIDL] & 0x03);
        *id = (*id<<8) + tbufdata[MCP_EID8];
        *id = (*id<<8) + tbufdata[MCP_EID0];
        *ext = 1;
    }
}

void mcp2515_write_can_msg(XSpiPs * SpiInstancePtr, u8 SlaveSelect,mcp_can_struct * mcp_can, u8 buffer_sidh_addr){
    u8 mcp_addr;
    mcp_addr = buffer_sidh_addr;
    mcp2515_set_reg_n(SpiInstancePtr, SlaveSelect, mcp_addr+5, mcp_can->m_nDta, mcp_can->m_nDlc );                  /* write data bytes             */

    if ( mcp_can->m_nRtr == 1)                                                   /* if RTR set bit in byte       */
        mcp_can->m_nDlc |= MCP_RTR_MASK;

    mcp2515_set_reg(SpiInstancePtr, SlaveSelect, (mcp_addr+4), mcp_can->m_nDlc );                         /* write the RTR and DLC        */
    mcp2515_write_id(SpiInstancePtr, SlaveSelect, mcp_addr, mcp_can->m_nExtFlg, mcp_can->m_nID );                      /* write CAN id                 */

}

void mcp2515_read_can_msg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 buffer_sidh_addr)        /* read can msg                 */
{
    u8 mcp_addr, ctrl;

    mcp_addr = buffer_sidh_addr;

    mcp2515_read_id(SpiInstancePtr, SlaveSelect, mcp_addr, &mcp_can->m_nExtFlg,&mcp_can->m_nID );

    ctrl = mcp2515_read_reg(SpiInstancePtr, SlaveSelect, mcp_addr-1 );
    mcp_can->m_nDlc = mcp2515_read_reg(SpiInstancePtr, SlaveSelect,  mcp_addr+4 );

    if (ctrl & 0x08)
        mcp_can->m_nRtr = 1;
    else
        mcp_can->m_nRtr = 0;

    mcp_can->m_nDlc &= MCP_DLC_MASK;
    mcp2515_read_reg_n( SpiInstancePtr, SlaveSelect, mcp_addr+5, &(mcp_can->m_nDta[0]), mcp_can->m_nDlc );
}

u8 mcp2515_get_next_free_TX_buf(XSpiPs *SpiInstancePtr, u8 SlaveSelect,  u8 *txbuf_n)                 /* get Next free txbuf          */
{
    u8 res, i, ctrlval;
    u8 ctrlregs[MCP_N_TXBUFFERS] = { MCP_TXB0CTRL, MCP_TXB1CTRL, MCP_TXB2CTRL };

    res = MCP_ALLTXBUSY;
    *txbuf_n = 0x00;

                                                                        /* check all 3 TX-Buffers       */
    for (i=0; i<MCP_N_TXBUFFERS; i++) {
        ctrlval = mcp2515_read_reg(SpiInstancePtr, SlaveSelect, ctrlregs[i] );
        if ( (ctrlval & MCP_TXB_TXREQ_M) == 0 ) {
            *txbuf_n = ctrlregs[i]+1;                                   /* return SIDH-address of Buffer*/

            res = MCP2515_OK;
            return res;                                                 /* ! function exit              */
        }
    }
    return res;
}

u8 mcp_can_set_msg(mcp_can_struct * mcp_can, unsigned int id, u8 rtr, u8 ext, u8 len, u8 *pData)
{
    int i = 0;
    mcp_can->m_nID     = id;
    mcp_can->m_nRtr    = rtr;
    mcp_can->m_nExtFlg = ext;
    mcp_can->m_nDlc    = len;
    for(i = 0; i<MAX_CHAR_IN_MESSAGE; i++)
        mcp_can->m_nDta[i] = *(pData+i);

    return MCP2515_OK;
}

u8 mcp_can_clear_msg(mcp_can_struct * mcp_can)
{
    mcp_can->m_nID       = 0;
    mcp_can->m_nDlc      = 0;
    mcp_can->m_nExtFlg   = 0;
    mcp_can->m_nRtr      = 0;
    mcp_can->m_nfilhit   = 0;
    for(int i = 0; i<mcp_can->m_nDlc; i++ )
      mcp_can->m_nDta[i] = 0x00;

    return MCP2515_OK;
}



u8 mcp2515_read_msg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can)
{
    u8 stat, res;

    stat = mcp2515_read_status(SpiInstancePtr, SlaveSelect);

    if ( stat & MCP_STAT_RX0IF )                                        /* Msg in Buffer 0              */
    {
        mcp2515_read_can_msg(SpiInstancePtr, SlaveSelect, mcp_can, MCP_RXBUF_0);
        mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_CANINTF, MCP_RX0IF, 0);
        res = CAN_OK;
    }
    else if ( stat & MCP_STAT_RX1IF )                                   /* Msg in Buffer 1              */
    {
    	mcp2515_read_can_msg(SpiInstancePtr, SlaveSelect, mcp_can,MCP_RXBUF_1);
    	mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_CANINTF, MCP_RX1IF, 0);
        res = CAN_OK;
    }
    else
        res = CAN_NOMSG;

    return res;
}

u8 mcp2515_check_receive(XSpiPs *SpiInstancePtr, u8 SlaveSelect)
{
    u8 res;
    res = mcp2515_read_status(SpiInstancePtr, SlaveSelect);                                         /* RXnIF in Bit 1 and 0         */
    if ( res & MCP_STAT_RXIF_MASK )
        return CAN_MSGAVAIL;
    else
        return CAN_NOMSG;
}

u8 mcp2515_read_msg_buf(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, unsigned int *id, u8 *ext, u8 *len, u8 buf[])
{
    if(mcp2515_read_msg(SpiInstancePtr, SlaveSelect, mcp_can) == CAN_NOMSG)
    	return CAN_NOMSG;

    *id  = mcp_can->m_nID;
    *len = mcp_can->m_nDlc;
    *ext = mcp_can->m_nExtFlg;
    for(int i = 0; i<mcp_can->m_nDlc; i++)
        buf[i] = mcp_can->m_nDta[i];

    return CAN_OK;
}

u8 mcp2515_read_msg_buf_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, unsigned int *id, u8 *len, u8 buf[])
{
    if(mcp2515_read_msg(SpiInstancePtr, SlaveSelect, mcp_can) == CAN_NOMSG)
	return CAN_NOMSG;

    if (mcp_can->m_nExtFlg)
        mcp_can->m_nID |= 0x80000000;

    if (mcp_can->m_nRtr)
        mcp_can->m_nID |= 0x40000000;

    *id  = mcp_can->m_nID;
    *len = mcp_can->m_nDlc;

    for(int i = 0; i<mcp_can->m_nDlc; i++)
        buf[i] = mcp_can->m_nDta[i];

    return CAN_OK;
}

u8 mcp2515_check_error(XSpiPs * SpiInstancePtr, u8 SlaveSelect)
{
    u8 eflg = mcp2515_read_reg(SpiInstancePtr, SlaveSelect, MCP_EFLG);

    if ( eflg & MCP_EFLG_ERRORMASK )
        return CAN_CTRLERROR;
    else
        return CAN_OK;
}

u8  mcp2515_get_error(XSpiPs * SpiInstancePtr, u8 SlaveSelect){
	return mcp2515_read_reg(SpiInstancePtr, SlaveSelect, MCP_EFLG);
}

u8 mcp2515_error_count_rx(XSpiPs * SpiInstancePtr, u8 SlaveSelect){
	return mcp2515_read_reg(SpiInstancePtr, SlaveSelect, MCP_REC);
}

u8 mcp2515_error_count_tx(XSpiPs * SpiInstancePtr, u8 SlaveSelect){
	return mcp2515_read_reg(SpiInstancePtr, SlaveSelect, MCP_TEC);
}

u8 mcp_can_en_one_shot_tx(XSpiPs * SpiInstancePtr, u8 SlaveSelect)
{
    mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_CANCTRL, MODE_ONESHOT, MODE_ONESHOT);
    if((mcp2515_read_reg(SpiInstancePtr, SlaveSelect, MCP_CANCTRL) & MODE_ONESHOT) != MODE_ONESHOT)
	    return CAN_FAIL;
    else
	    return CAN_OK;
}

u8 mcp2515_dis_one_shot_tx(XSpiPs * SpiInstancePtr, u8 SlaveSelect)
{
    mcp2515_modify_reg(SpiInstancePtr, SlaveSelect, MCP_CANCTRL, MODE_ONESHOT, 0);
    if((mcp2515_read_reg(SpiInstancePtr, SlaveSelect, MCP_CANCTRL) & MODE_ONESHOT) != 0)
        return CAN_FAIL;
    else
        return CAN_OK;
}

u8 mcp2515_init_mask(XSpiPs * SpiInstancePtr, u8 SlaveSelect,mcp_can_struct * mcp_can, u8 num, u8 ext, unsigned int ulData)
{
    u8 res = MCP2515_OK;
    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, MODE_CONFIG);
    if(res > 0){
    	xil_printf("Entering Configuration Mode Failure...\r\n");
    	return res;
    }

    if (num == 0){
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXM0SIDH, ext, ulData);

    }
    else if(num == 1){
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXM1SIDH, ext, ulData);
    }
    else res =  MCP2515_FAIL;

    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, mcp_can->mcpMode);
    if(res > 0){
    	xil_printf("Entering Previous Mode Failure...\r\nSetting Mask Failure...\r\n");
    	return res;
    }
    return res;
}

u8 mcp2515_init_mask_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 num, unsigned int ulData)
{
    u8 res = MCP2515_OK;
    u8 ext = 0;
    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, MODE_CONFIG);
    if(res > 0){
    	xil_printf("Entering Configuration Mode Failure...\r\n");
    	return res;
    }
    if((num & 0x80000000) == 0x80000000)
        ext = 1;

    if (num == 0){
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXM0SIDH, ext, ulData);

    }
    else if(num == 1){
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXM1SIDH, ext, ulData);
    }
    else res =  MCP2515_FAIL;

    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, mcp_can->mcpMode);
    if(res > 0){
    	xil_printf("Entering Previous Mode Failure...\r\nSetting Mask Failure...\r\n");
    	return res;
    }
    return res;
}

u8 mcp2515_init_filt(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 num, u8 ext, unsigned int ulData)
{
    u8 res = MCP2515_OK;
    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, MODE_CONFIG);
    if(res > 0)
    {
      xil_printf("Enter Configuration Mode Failure...\r\n");
      return res;
    }

    switch( num )
    {
        case 0:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF0SIDH, ext, ulData);
        break;

        case 1:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF1SIDH, ext, ulData);
        break;

        case 2:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF2SIDH, ext, ulData);
        break;

        case 3:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF3SIDH, ext, ulData);
        break;

        case 4:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF4SIDH, ext, ulData);
        break;

        case 5:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF5SIDH, ext, ulData);
        break;

        default:
        res = MCP2515_FAIL;
    }

    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, mcp_can->mcpMode);
    if(res > 0)
    {
      xil_printf("Entering Previous Mode Failure...\r\nSetting Filter Failure...\r\n");
      return res;
    }
    return res;
}

u8 mcp2515_init_filt_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, u8 num, unsigned int ulData)
{
    u8 res = MCP2515_OK;
    u8 ext = 0;

    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, MODE_CONFIG);
    if(res > 0)
    {
    	xil_printf("Enter Configuration Mode Failure...\r\n");
    	return res;
    }

    if((num & 0x80000000) == 0x80000000)
        ext = 1;

    switch( num )
    {
        case 0:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF0SIDH, ext, ulData);
        break;

        case 1:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF1SIDH, ext, ulData);
        break;

        case 2:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF2SIDH, ext, ulData);
        break;

        case 3:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF3SIDH, ext, ulData);
        break;

        case 4:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF4SIDH, ext, ulData);
        break;

        case 5:
        mcp2515_write_mf(SpiInstancePtr, SlaveSelect, MCP_RXF5SIDH, ext, ulData);
        break;

        default:
        res = MCP2515_FAIL;
    }

    res = mcp2515_set_CANCTRL_mode(SpiInstancePtr, SlaveSelect, mcp_can->mcpMode);
    if(res > 0)
    {
    	xil_printf("Entering Previous Mode Failure...\r\nSetting Filter Failure...\r\n");
    	return res;
    }
    return res;
}

u8 mcp2515_send_msg(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can)
{
    u8 res, res1, txbuf_n;
    uint16_t uiTimeOut = 0;

    do {
        res = mcp2515_get_next_free_TX_buf(SpiInstancePtr, SlaveSelect, &txbuf_n);                       /* info = addr.                 */
        uiTimeOut++;
    } while (res == MCP_ALLTXBUSY && (uiTimeOut < TIMEOUTVALUE));

    if(uiTimeOut == TIMEOUTVALUE)
    {
        return CAN_GETTXBFTIMEOUT;                                      /* get tx buff time out         */
    }
    uiTimeOut = 0;
    mcp2515_write_can_msg(SpiInstancePtr, SlaveSelect,mcp_can, txbuf_n);
    mcp2515_modify_reg( SpiInstancePtr, SlaveSelect, txbuf_n-1 , MCP_TXB_TXREQ_M, MCP_TXB_TXREQ_M );

    do
    {
        uiTimeOut++;
        res1 = mcp2515_read_reg(SpiInstancePtr, SlaveSelect, txbuf_n-1);                         /* read send buff ctrl reg 	*/
        res1 = res1 & 0x08;
    } while (res1 && (uiTimeOut < TIMEOUTVALUE));

    if(uiTimeOut == TIMEOUTVALUE)                                       /* send msg timeout             */
        return CAN_SENDMSGTIMEOUT;

    return CAN_OK;
}

u8 mcp2515_send_msg_buf(XSpiPs * SpiInstancePtr, u8 SlaveSelect, mcp_can_struct * mcp_can, unsigned int id, u8 ext, u8 len, u8 *buf)
{
    u8 res;

    mcp_can_set_msg(mcp_can, id, 0, ext, len, buf);
    res = mcp2515_send_msg(SpiInstancePtr, SlaveSelect, mcp_can);

    return res;
}

u8 mcp2515_send_msg_buf_no_ext(XSpiPs * SpiInstancePtr, u8 SlaveSelect,mcp_can_struct * mcp_can,  unsigned int id, u8 len, u8 *buf)
{
    u8 ext = 0, rtr = 0;
    u8 res;

    if((id & 0x80000000) == 0x80000000)
        ext = 1;

    if((id & 0x40000000) == 0x40000000)
        rtr = 1;

    mcp_can_set_msg(mcp_can, id, rtr, ext, len, buf);
    res = mcp2515_send_msg(SpiInstancePtr, SlaveSelect, mcp_can);

    return res;
}
