#include "inc/can.h"
#include "device.h"
#include "std_errno.h"
#include "mpu_types.h"
#include "cpuemu_ops.h"
#include "athrill_mros_device.h"
#include <stdio.h>

static Std_ReturnType can_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data);
static Std_ReturnType can_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data);
static Std_ReturnType can_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data);
static Std_ReturnType can_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data);
static Std_ReturnType can_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data);
static Std_ReturnType can_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data);
static Std_ReturnType can_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data);

MpuAddressRegionOperationType	can_memory_operation = {
		.get_data8 		= 	can_get_data8,
		.get_data16		=	can_get_data16,
		.get_data32		=	can_get_data32,

		.put_data8 		= 	can_put_data8,
		.put_data16		=	can_put_data16,
		.put_data32		=	can_put_data32,

		.get_pointer	= can_get_pointer,
};

#define MROS_PUB_REQ_NUM	2
#define MROS_SUB_REQ_NUM	2
static AthrillMrosDevPubReqType pub_req_table[MROS_PUB_REQ_NUM] = {
		{ .topic_name = "CANID_0x100", .pub = NULL },
		{ .topic_name = "CANID_0x101", .pub = NULL },
};
static AthrillMrosDevSubReqType sub_req_table[MROS_SUB_REQ_NUM] = {
		{ .topic_name = "CANID_0x400", .sub = NULL, .callback = NULL, .sub = NULL },
		{ .topic_name = "CANID_0x404", .sub = NULL, .callback = NULL, .sub = NULL },
};

void device_init_can(MpuAddressRegionType *region)
{
	int err;
	set_athrill_task();
	err = athrill_mros_device_pub_register(pub_req_table, MROS_PUB_REQ_NUM);
	if (err < 0) {
		printf("error: athrill_mros_device_pub_register()\n");
		return;
	}
	err = athrill_mros_device_sub_register(sub_req_table, MROS_SUB_REQ_NUM);
	if (err < 0) {
		printf("error: athrill_mros_device_sub_register()\n");
		return;
	}
	err = athrill_mros_device_start();
	if (err < 0) {
		printf("error: athrill_mros_device_start()\n");
		return;
	}
	return;
}


static Std_ReturnType can_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint8*)(&region->data[off]));

	if ((addr >= VCAN_RX_FLAG_BASE) && (addr < (VCAN_RX_FLAG_BASE + VCAN_RX_FLAG_SIZE))) {
		//TODO
	}
	return STD_E_OK;
}
static Std_ReturnType can_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint16*)(&region->data[off]));
	return STD_E_OK;
}
static Std_ReturnType can_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint32*)(&region->data[off]));
	if ((addr >= VCAN_RX_DATA_BASE) && (addr < (VCAN_RX_DATA_BASE + VCAN_RX_DATA_SIZE))) {
		//TODO
	}
	return STD_E_OK;
}
static Std_ReturnType can_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data)
{
	uint32 off = (addr - region->start);
	*((uint8*)(&region->data[off])) = data;

	if ((addr >= VCAN_TX_FLAG_BASE) && (addr < (VCAN_TX_FLAG_BASE + VCAN_TX_FLAG_SIZE))) {
		//TODO
	}

	return STD_E_OK;
}
static Std_ReturnType can_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data)
{
	uint32 off = (addr - region->start);
	*((uint16*)(&region->data[off])) = data;
	return STD_E_OK;
}
static Std_ReturnType can_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data)
{
	uint32 off = (addr - region->start);
	*((uint32*)(&region->data[off])) = data;
	if ((addr >= VCAN_TX_DATA_BASE) && (addr < (VCAN_TX_DATA_BASE + VCAN_TX_DATA_SIZE))) {
		//TODO
	}
	return STD_E_OK;
}
static Std_ReturnType can_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data)
{
	uint32 off = (addr - region->start);
	*data = &region->data[off];
	return STD_E_OK;
}
