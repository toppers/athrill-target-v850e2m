#include "inc/can.h"
#include "device.h"
#include "std_errno.h"
#include "mpu_types.h"
#include "cpuemu_ops.h"
#include "athrill_mros_device.h"
#include "mpu_ops.h"
#include "assert.h"
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

typedef enum {
	mRosTopicType_PUB = 0,
	mRosTopicType_SUB
} mRosTopicType;

#define MROS_SUB_REQ_NUM	2
static uint32 mros_pub_req_num = 0;
static AthrillMrosDevPubReqType *pub_req_table;

static void topic_callback(const char *data, int datalen);
static uint32 mros_sub_req_num = 0;
static AthrillMrosDevSubReqType *sub_req_table;

static void device_parse_can_config(mRosTopicType type, const char* fmt_num, const char* fmt_topic)
{
	uint32 i;
	uint32 param_num;
	Std_ReturnType err;
	static char parameter[4096];
	char *param_value;

	err = cpuemu_get_devcfg_value(fmt_num, &param_num);
	if (err != STD_E_OK) {
		return;
	}
	if (type == mRosTopicType_PUB) {
		mros_pub_req_num = param_num;
		pub_req_table = malloc(sizeof(AthrillMrosDevPubReqType) * param_num);
		ASSERT(pub_req_table != NULL);
	}
	else {
		mros_sub_req_num = param_num;
		sub_req_table = malloc(sizeof(AthrillMrosDevSubReqType) * param_num);
		ASSERT(sub_req_table != NULL);
	}
	for (i = 0; i < param_num; i++) {
		snprintf(parameter, sizeof(parameter), fmt_topic, i);
		err = cpuemu_get_devcfg_string(parameter, &param_value);
		if (err != STD_E_OK) {
			printf("not found param=%s\n", parameter);
			return;
		}
		if (type == mRosTopicType_PUB) {
			pub_req_table[i].topic_name = param_value;
			pub_req_table[i].pub = NULL;
		}
		else {
			sub_req_table[i].topic_name = param_value;
			sub_req_table[i].callback = topic_callback;
			sub_req_table[i].sub = NULL;
		}
	}
}
static MpuAddressRegionType *can_data_region;
static void topic_callback_common(const char *data, int datalen, uint32 rx_data_addr, uint32 rx_datalen_addr, uint32 rx_flag_addr)
{
	uint32 copylen = datalen;
	uint32 *tmp_data_addr;
	char *tmp_data;
	uint32 *tmp_datalen;
	uint8 *rx_flag;

	(void)mpu_get_pointer(0, rx_data_addr, (uint8 **)&tmp_data_addr);
	(void)mpu_get_pointer(0, (*tmp_data_addr), (uint8 **)&tmp_data);
	(void)mpu_get_pointer(0, rx_datalen_addr, (uint8 **)&tmp_datalen);
	//printf("topic_callback_01:data=%s datalen=%d\n", tmp_data, *tmp_datalen);
	if (*tmp_datalen == 0) {
		return;
	}
	memcpy(tmp_data, data, copylen);
	if (copylen > *tmp_datalen) {
		copylen = (*tmp_datalen);
	}
	if ((*tmp_datalen) > copylen) {
		tmp_data[copylen] = '\0';
	}
	else {
		tmp_data[copylen - 1] = '\0';
	}
	(void)mpu_get_pointer(0, rx_flag_addr, (uint8 **)&rx_flag);
	*rx_flag = 1;
	return;
}
static void topic_callback(const char *data, int datalen)
{
	mRosCallbackTopicIdType topic_id = ros_topic_callback_topic_id();
	int i;
	for (i = 0; i < mros_sub_req_num; i++) {
		if (sub_req_table[i].sub == NULL) {
			continue;
		}
		if (topic_id == sub_req_table[i].sub->topic_id) {
			break;
		}
	}
	if (i == mros_sub_req_num) {
		return;
	}
	topic_callback_common(data, datalen, VCAN_RX_DATA_0(i), VCAN_RX_DATA_1(i), VCAN_RX_FLAG(i));
	return;
}

void device_init_can(MpuAddressRegionType *region)
{
	int err;
	can_data_region = region;
	set_athrill_task();

	device_parse_can_config(mRosTopicType_PUB, "DEBUG_FUNC_MROS_TOPIC_PUB_NUM", "DEBUG_FUNC_MROS_TOPIC_PUB_%d");
	device_parse_can_config(mRosTopicType_SUB, "DEBUG_FUNC_MROS_TOPIC_SUB_NUM", "DEBUG_FUNC_MROS_TOPIC_SUB_%d");

	err = athrill_mros_device_pub_register(pub_req_table, mros_pub_req_num);
	if (err < 0) {
		printf("error: athrill_mros_device_pub_register()\n");
		return;
	}
	err = athrill_mros_device_sub_register(sub_req_table, mros_sub_req_num);
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
	return STD_E_OK;
}
static Std_ReturnType can_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data)
{
	uint32 off = (addr - region->start);
	uint32 mbox;
	*((uint8*)(&region->data[off])) = data;

	if ((addr >= VCAN_TX_FLAG_BASE) && (addr < (VCAN_TX_FLAG_BASE + VCAN_TX_FLAG_SIZE))) {
		mbox = addr - VCAN_TX_FLAG_BASE;
		if (mbox <= VCAN_MAX_MBOX_NUM) {
			uint32 *tmp_data_addr;
			char *tmp_data;
			uint32 *tmp_datalen;
			(void)mpu_get_pointer(0, VCAN_TX_DATA_0(mbox) , (uint8 **)&tmp_data_addr);
			(void)mpu_get_pointer(0, (*tmp_data_addr), (uint8 **)&tmp_data);
			(void)mpu_get_pointer(0, VCAN_TX_DATA_1(mbox) , (uint8 **)&tmp_datalen);
			ros_topic_publish(pub_req_table[mbox].pub, (void*)tmp_data, (sint32)*tmp_datalen);
		}
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
	return STD_E_OK;
}
static Std_ReturnType can_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data)
{
	uint32 off = (addr - region->start);
	*data = &region->data[off];
	return STD_E_OK;
}
