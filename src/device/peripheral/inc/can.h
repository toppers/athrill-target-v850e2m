#ifndef _CAN_H_
#define _CAN_H_

#include "device.h"

#define VCAN_BASE			0x08FF0000
#define VCAN_RX_DATA_BASE	0x08FF0000
#define VCAN_RX_DATA_SIZE	0x1000
#define VCAN_RX_FLAG_BASE	0x08FF1000
#define VCAN_RX_FLAG_SIZE	0x1000

#define VCAN_TX_DATA_BASE	0x08FF2000
#define VCAN_TX_DATA_SIZE	0x1000
#define VCAN_TX_FLAG_BASE	0x08FF3000
#define VCAN_TX_FLAG_SIZE	0x1000

#define VCAN_MAX_MBOX_NUM		1024
/*
 * RX CAN DATA ADDR
 */
#define VCAN_RX_DATA_0(mbox)	(VCAN_RX_DATA_BASE + ( ( 8 * (mbox) + 0 ) ))
#define VCAN_RX_DATA_1(mbox)	(VCAN_RX_DATA_BASE + ( ( 8 * (mbox) + 4 ) ))


/*
 * RX CAN FLAG ADDR
 */
#define VCAN_RX_FLAG(mbox)	(VCAN_RX_FLAG_BASE + (mbox) )

/*
 * TX CAN DATA ADDR
 */
#define VCAN_TX_DATA_0(mbox)	(VCAN_TX_DATA_BASE + ( ( 8 * (mbox) + 0 ) ))
#define VCAN_TX_DATA_1(mbox)	(VCAN_TX_DATA_BASE + ( ( 8 * (mbox) + 4 ) ))

/*
 * TX CAN FLAG ADDR
 */
#define VCAN_TX_FLAG(mbox)	(VCAN_TX_FLAG_BASE + (mbox) )

#endif /* _CAN_H_ */
