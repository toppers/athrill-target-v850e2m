#include "vdev_private.h"
#include "cpuemu_ops.h"
#include "mpu_ops.h"

static Std_ReturnType vdev_mmap_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data);
static Std_ReturnType vdev_mmap_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data);
static Std_ReturnType vdev_mmap_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data);
static Std_ReturnType vdev_mmap_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data);
static Std_ReturnType vdev_mmap_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data);
static Std_ReturnType vdev_mmap_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data);
static Std_ReturnType vdev_mmap_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data);

MpuAddressRegionOperationType	vdev_mmap_memory_operation = {
		.get_data8 		= 	vdev_mmap_get_data8,
		.get_data16		=	vdev_mmap_get_data16,
		.get_data32		=	vdev_mmap_get_data32,

		.put_data8 		= 	vdev_mmap_put_data8,
		.put_data16		=	vdev_mmap_put_data16,
		.put_data32		=	vdev_mmap_put_data32,

		.get_pointer	= vdev_mmap_get_pointer,
};

void device_init_vdev_mmap(MpuAddressRegionType *region)
{
	Std_ReturnType err;
	err = cpuemu_get_devcfg_value_hex("DEBUG_FUNC_VDEV_MMAP_TX", &vdev_control.tx_data_addr);
	if (err != STD_E_OK) {
		printf("ERROR: can not load param DEBUG_FUNC_VDEV_MMAP_TX\n");
		ASSERT(err == STD_E_OK);
	}
	vdev_control.tx_data = mpu_address_get_ram(vdev_control.tx_data_addr, 1U);
	ASSERT(vdev_control.tx_data != NULL);

	err = cpuemu_get_devcfg_value_hex("DEBUG_FUNC_VDEV_MMAP_RX", &vdev_control.rx_data_addr);
	if (err != STD_E_OK) {
		printf("ERROR: can not load param DEBUG_FUNC_VDEV_MMAP_RX\n");
		ASSERT(err == STD_E_OK);
	}
	vdev_control.rx_data = mpu_address_get_ram(vdev_control.rx_data_addr, 1U);
	ASSERT(vdev_control.rx_data != NULL);
	return;
}

void device_supply_clock_vdev_mmap(DeviceClockType *dev_clock)
{
	uint64 interval;
	uint64 unity_sim_time;

	uint64 curr_stime;
	memcpy((void*)&curr_stime, &vdev_control.rx_data[VDEV_SIM_TIME(VDEV_SIM_INX_ME)], 8U);
	vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU] = curr_stime;

#if 1
	unity_sim_time = vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU] * ((uint64)vdev_control.cpu_freq);
#else
	unity_sim_time = vdev_get_unity_sim_time(dev_clock);
#endif

	vdev_control.vdev_sim_time[VDEV_SIM_INX_ME] = ( dev_clock->clock / ((uint64)vdev_control.cpu_freq) );

	if ((unity_sim_time != 0) && (dev_clock->min_intr_interval != DEVICE_CLOCK_MAX_INTERVAL)) {
		if ((unity_sim_time <= dev_clock->clock)) {
			interval = 2U;
			//printf("UNITY <= MICON:%llu %llu\n", vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU], vdev_control.vdev_sim_time[VDEV_SIM_INX_ME]);
		}
		else {
			//interval = (unity_sim_time - dev_clock->clock) + ((unity_interval_vtime  * ((uint64)vdev_udp_control.cpu_freq)) / 2);
			interval = (unity_sim_time - dev_clock->clock);
			//printf("UNITY > MICON:%llu %llu\n", vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU], vdev_control.vdev_sim_time[VDEV_SIM_INX_ME]);
		}
		if (interval < dev_clock->min_intr_interval) {
			dev_clock->min_intr_interval = interval;
		}
	}
	memcpy(&vdev_control.tx_data[VDEV_SIM_TIME(VDEV_SIM_INX_ME)],  (void*)&vdev_control.vdev_sim_time[VDEV_SIM_INX_ME], 8U);
	memcpy(&vdev_control.tx_data[VDEV_SIM_TIME(VDEV_SIM_INX_YOU)], (void*)&vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU], 8U);
	return;
}

static Std_ReturnType vdev_mmap_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint8*)(&vdev_control.rx_data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_mmap_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint16*)(&vdev_control.rx_data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_mmap_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data)
{
	uint32 off = (addr - region->start);
	*data = *((uint32*)(&vdev_control.rx_data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_mmap_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data)
{

	if (addr == VDEV_TX_FLAG(0)) {
	}
	else {
		uint32 off = addr - VDEV_TX_DATA_BASE;
		*((uint8*)(&vdev_control.tx_data[off])) = data;
	}

	return STD_E_OK;
}
static Std_ReturnType vdev_mmap_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data)
{
	uint32 off = addr - VDEV_TX_DATA_BASE;
	*((uint16*)(&vdev_control.tx_data[off])) = data;
	return STD_E_OK;
}
static Std_ReturnType vdev_mmap_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data)
{
	uint32 off = addr - VDEV_TX_DATA_BASE;
	*((uint32*)(&vdev_control.tx_data[off])) = data;
	return STD_E_OK;
}
static Std_ReturnType vdev_mmap_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data)
{
	uint32 off = (addr - region->start);
	*data = &region->data[off];
	return STD_E_OK;
}
