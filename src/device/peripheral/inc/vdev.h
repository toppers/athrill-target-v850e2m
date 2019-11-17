#ifndef _VDEV_H_
#define _VDEV_H_

#define VDEV_BASE			0x090F0000

#define VDEV_RX_DATA_BASE	VDEV_BASE
#define VDEV_RX_DATA_SIZE	0x1000

#define VDEV_TX_DATA_BASE	(VDEV_BASE + VDEV_RX_DATA_SIZE)
#define VDEV_TX_DATA_SIZE	0x1000

#define VDEV_TX_FLAG_BASE	(VDEV_TX_DATA_BASE + VDEV_TX_DATA_SIZE)
#define VDEV_TX_FLAG_SIZE	0x1000

#define VDEV_SIM_TIME_BASE	0xF00
#define VDEV_SIM_TIME_SIZE	0x100

#define VDEV_SIM_TIME(inx)		( VDEV_SIM_TIME_BASE + ( (inx) * 8U ) )

#define VDEV_SIM_INX_NUM		2U
#define VDEV_SIM_INX_ME			0U
#define VDEV_SIM_INX_YOU		1U

/*
 * RX VDEV DATA ADDR
 */
#define VDEV_RX_DATA(index)	(VDEV_RX_DATA_BASE + ( ( 4 * (index) + 0 ) ))

/*
 * TX VDEV DATA ADDR
 */
#define VDEV_TX_DATA(index)	(VDEV_TX_DATA_BASE + ( ( 4 * (index) + 0 ) ))

/*
 * TX VDEV FLAG ADDR
 */
#define VDEV_TX_FLAG(index)	(VDEV_TX_FLAG_BASE + ( ( 1 * (index) + 0 ) ))


#endif /* _VDEV_H_ */
