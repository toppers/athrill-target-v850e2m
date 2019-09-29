#include "device.h"
#include "inc/vdev.h"
#include "device.h"
#include "std_errno.h"
#include "mpu_types.h"
#include "cpuemu_ops.h"
#include "athrill_mpthread.h"
#include "mpu_ops.h"
#include "udp/udp_comm.h"
#include "assert.h"
#include <string.h>
#include <stdio.h>

static Std_ReturnType vdev_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data);
static Std_ReturnType vdev_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data);
static Std_ReturnType vdev_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data);
static Std_ReturnType vdev_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data);
static Std_ReturnType vdev_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data);
static Std_ReturnType vdev_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data);
static Std_ReturnType vdev_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data);

MpuAddressRegionOperationType	vdev_memory_operation = {
		.get_data8 		= 	vdev_get_data8,
		.get_data16		=	vdev_get_data16,
		.get_data32		=	vdev_get_data32,

		.put_data8 		= 	vdev_put_data8,
		.put_data16		=	vdev_put_data16,
		.put_data32		=	vdev_put_data32,

		.get_pointer	= vdev_get_pointer,
};

Std_ReturnType mpthread_init(void);
extern Std_ReturnType mpthread_register(MpthrIdType *id, MpthrOperationType *op);

static MpthrIdType vdev_thrid;

typedef struct {
	UdpCommConfigType config;
	UdpCommType comm;
	MpuAddressRegionType *region;
} VdevUdpControlType;

static Std_ReturnType vdev_thread_do_init(MpthrIdType id);
static Std_ReturnType vdev_thread_do_proc(MpthrIdType id);
static MpthrOperationType vdev_op = {
	.do_init = vdev_thread_do_init,
	.do_proc = vdev_thread_do_proc,
};

static VdevUdpControlType vdev_udp_control;

void device_init_vdev(MpuAddressRegionType *region)
{
	Std_ReturnType err;
	uint32 portno;

	vdev_udp_control.region = region;
	vdev_udp_control.config.is_wait = TRUE;

	err = cpuemu_get_devcfg_value("DEBUG_FUNC_VDEV_TX_PORTNO", &portno);
	if (err != STD_E_OK) {
		printf("ERROR: can not load param DEBUG_FUNC_VDEV_TX_PORTNO\n");
		ASSERT(err == STD_E_OK);
	}
	vdev_udp_control.config.client_port = (uint16)portno;
	err = cpuemu_get_devcfg_value("DEBUG_FUNC_VDEV_RX_PORTNO", &portno);
	if (err != STD_E_OK) {
		printf("ERROR: can not load param DEBUG_FUNC_VDEV_RX_PORTNO\n");
		ASSERT(err == STD_E_OK);
	}
	vdev_udp_control.config.server_port = (uint16)portno;

	err = udp_comm_create(&vdev_udp_control.config, &vdev_udp_control.comm);
	ASSERT(err == STD_E_OK);

	mpthread_init();

	err = mpthread_register(&vdev_thrid, &vdev_op);
	ASSERT(err == STD_E_OK);

	err = mpthread_start_proc(vdev_thrid);
	ASSERT(err == STD_E_OK);
	return;
}

void device_supply_clock_vdev(DeviceClockType *dev_clock)
{
	//nothing to do
	return;
}

static Std_ReturnType vdev_thread_do_init(MpthrIdType id)
{
	//nothing to do
	return STD_E_OK;
}
static Std_ReturnType vdev_thread_do_proc(MpthrIdType id)
{
	Std_ReturnType err;
	uint32 off = VDEV_RX_DATA_BASE - VDEV_BASE;
	while (1) {
		err = udp_comm_read(&vdev_udp_control.comm);
		if (err != STD_E_OK) {
			continue;
		}
		memcpy(&vdev_udp_control.region->data[off], &vdev_udp_control.comm.read_data.buffer[0], vdev_udp_control.comm.read_data.len);
	}
	return STD_E_OK;
}



static Std_ReturnType vdev_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint8*)(&region->data[off]));

	return STD_E_OK;
}
static Std_ReturnType vdev_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint16*)(&region->data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint32*)(&region->data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data)
{
	Std_ReturnType err;
	uint32 off = (addr - region->start);
	*((uint8*)(&region->data[off])) = data;
	uint32 tx_off = VDEV_TX_DATA_BASE - region->start;

	if (addr == VCAN_TX_FLAG(0)) {
		memcpy(&vdev_udp_control.comm.write_data.buffer[0], &region->data[tx_off], UDP_BUFFER_LEN);
		vdev_udp_control.comm.write_data.len = UDP_BUFFER_LEN;
		err = udp_comm_write(&vdev_udp_control.comm);
		if (err != STD_E_OK) {
			printf("WARNING: vdevput_data8: udp send error=%d\n", err);
		}
	}

	return STD_E_OK;
}
static Std_ReturnType vdev_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data)
{
	uint32 off = (addr - region->start);
	*((uint16*)(&region->data[off])) = data;
	return STD_E_OK;
}
static Std_ReturnType vdev_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data)
{
	uint32 off = (addr - region->start);
	*((uint32*)(&region->data[off])) = data;
	return STD_E_OK;
}
static Std_ReturnType vdev_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data)
{
	uint32 off = (addr - region->start);
	*data = &region->data[off];
	return STD_E_OK;
}

