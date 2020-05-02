#include "vdev_private.h"
#include "athrill_mpthread.h"
#include "cpuemu_ops.h"

static Std_ReturnType vdev_udp_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data);
static Std_ReturnType vdev_udp_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data);
static Std_ReturnType vdev_udp_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data);
static Std_ReturnType vdev_udp_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data);
static Std_ReturnType vdev_udp_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data);
static Std_ReturnType vdev_udp_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data);
static Std_ReturnType vdev_udp_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data);

MpuAddressRegionOperationType	vdev_udp_memory_operation = {
		.get_data8 		= 	vdev_udp_get_data8,
		.get_data16		=	vdev_udp_get_data16,
		.get_data32		=	vdev_udp_get_data32,

		.put_data8 		= 	vdev_udp_put_data8,
		.put_data16		=	vdev_udp_put_data16,
		.put_data32		=	vdev_udp_put_data32,

		.get_pointer	= vdev_udp_get_pointer,
};


static MpthrIdType vdev_thrid;
Std_ReturnType mpthread_init(void);
extern Std_ReturnType mpthread_register(MpthrIdType *id, MpthrOperationType *op);

static Std_ReturnType vdev_thread_do_init(MpthrIdType id);
static Std_ReturnType vdev_thread_do_proc(MpthrIdType id);
static MpthrOperationType vdev_op = {
	.do_init = vdev_thread_do_init,
	.do_proc = vdev_thread_do_proc,
};

void device_init_vdev_udp(MpuAddressRegionType *region)
{
	Std_ReturnType err;
	uint32 portno;

	vdev_control.remote_ipaddr = "127.0.0.1";
	(void)cpuemu_get_devcfg_string("DEBUG_FUNC_VDEV_TX_IPADDR", &vdev_control.remote_ipaddr);
	printf("VDEV:TX IPADDR=%s\n", vdev_control.remote_ipaddr);
	err = cpuemu_get_devcfg_value("DEBUG_FUNC_VDEV_TX_PORTNO", &portno);
	if (err != STD_E_OK) {
		printf("ERROR: can not load param DEBUG_FUNC_VDEV_TX_PORTNO\n");
		ASSERT(err == STD_E_OK);
	}
	printf("VDEV:TX PORTNO=%d\n", portno);
	vdev_control.local_ipaddr = "127.0.0.1";
	(void)cpuemu_get_devcfg_string("DEBUG_FUNC_VDEV_RX_IPADDR", &vdev_control.local_ipaddr);
	printf("VDEV:RX IPADDR=%s\n", vdev_control.local_ipaddr);
	vdev_control.config.client_port = (uint16)portno;
	err = cpuemu_get_devcfg_value("DEBUG_FUNC_VDEV_RX_PORTNO", &portno);
	if (err != STD_E_OK) {
		printf("ERROR: can not load param DEBUG_FUNC_VDEV_RX_PORTNO\n");
		ASSERT(err == STD_E_OK);
	}
	vdev_control.config.server_port = (uint16)portno;
	printf("VDEV:RX PORTNO=%d\n", portno);

	err = udp_comm_create_ipaddr(&vdev_control.config, &vdev_control.comm, vdev_control.local_ipaddr);
	ASSERT(err == STD_E_OK);
	//initialize udp write buffer header
	{
		VdevTxDataHeadType *tx_headp = (VdevTxDataHeadType*)&vdev_control.comm.write_data.buffer[0];
		memset((void*)tx_headp, 0, VDEV_TX_DATA_HEAD_SIZE);
		memcpy((void*)tx_headp->header, VDEV_TX_DATA_HEAD_HEADER, strlen(VDEV_TX_DATA_HEAD_HEADER));
		tx_headp->version = VDEV_TX_DATA_HEAD_VERSION;
		tx_headp->ext_off = VDEV_TX_DATA_HEAD_EXT_OFF;
		tx_headp->ext_size = VDEV_TX_DATA_HEAD_EXT_SIZE;
		vdev_control.comm.write_data.len = VDEV_TX_DATA_COMM_SIZE;
	}
	mpthread_init();

	err = mpthread_register(&vdev_thrid, &vdev_op);
	ASSERT(err == STD_E_OK);

	err = mpthread_start_proc(vdev_thrid);
	ASSERT(err == STD_E_OK);
	return;
}
void device_supply_clock_vdev_udp(DeviceClockType *dev_clock)
{
	uint64 interval;
	uint64 unity_sim_time;

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
	return;
}

static Std_ReturnType vdev_thread_do_init(MpthrIdType id)
{
	//nothing to do
	return STD_E_OK;
}

static Std_ReturnType vdev_udp_packet_check(const char *p)
{
	const uint32 *p_int = (const uint32 *)&vdev_control.comm.read_data.buffer[0];
#if 0
	printf("HEADER:%c%c%c%c\n", p[0], p[1], p[2], p[3]);
	printf("version:0x%x\n", p_int[1]);
	printf("reserve[0]:0x%x\n", p_int[2]);
	printf("reserve[1]:0x%x\n", p_int[3]);
	printf("unity_time[0]:0x%x\n", p_int[4]);
	printf("unity_time[1]:0x%x\n", p_int[5]);
	printf("ext_off:0x%x\n", p_int[6]);
	printf("ext_size:0x%x\n", p_int[7]);
#endif
	if (strncmp(p, VDEV_RX_DATA_HEAD_HEADER, 4) != 0) {
		printf("ERROR: INVALID HEADER:%c%c%c%c\n", p[0], p[1], p[2], p[3]);
		return STD_E_INVALID;
	}
	if (p_int[1] != VDEV_RX_DATA_HEAD_VERSION) {
		printf("ERROR: INVALID VERSION:0x%x\n", p_int[1]);
		return STD_E_INVALID;
	}
	if (p_int[6] != VDEV_RX_DATA_HEAD_EXT_OFF) {
		printf("ERROR: INVALID EXT_OFF:0x%x\n", p_int[6]);
		return STD_E_INVALID;
	}
	if (p_int[7] != VDEV_RX_DATA_HEAD_EXT_SIZE) {
		printf("ERROR: INVALID EXT_SIZE:0x%x\n", p_int[7]);
		return STD_E_INVALID;
	}
	return STD_E_OK;
}

static Std_ReturnType vdev_thread_do_proc(MpthrIdType id)
{
	Std_ReturnType err;
	uint32 off = VDEV_RX_DATA_BASE - VDEV_BASE;
	uint64 curr_stime;

	while (1) {
		err = udp_comm_read(&vdev_control.comm);
		if (err != STD_E_OK) {
			continue;
		}
		if (vdev_udp_packet_check((const char*)&vdev_control.comm.read_data.buffer[0]) != STD_E_OK) {
			continue;
		}
		//gettimeofday(&unity_notify_time, NULL);
		memcpy(&vdev_control.region->data[off], &vdev_control.comm.read_data.buffer[0], vdev_control.comm.read_data.len);
		memcpy((void*)&curr_stime, &vdev_control.comm.read_data.buffer[VDEV_RX_SIM_TIME(VDEV_SIM_INX_ME)], 8U);

		//unity_interval_vtime = curr_stime - vdev_udp_control.vdev_sim_time[VDEV_SIM_INX_YOU];
		//vdev_calc_predicted_virtual_time(vdev_udp_control.vdev_sim_time[VDEV_SIM_INX_YOU], curr_stime);
		vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU] = curr_stime;
#if 0
		{
			uint32 count = 0;
			if ((count % 1000) == 0) {
				printf("%llu, %llu\n", vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU], vdev_control.vdev_sim_time[VDEV_SIM_INX_ME]);
			}
			count++;
		}
#endif
	}
	return STD_E_OK;
}


static Std_ReturnType vdev_udp_get_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 *data)
{
	uint32 off = (addr - region->start) + VDEV_RX_DATA_BODY_OFF;
	*data = *((uint8*)(&region->data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_udp_get_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 *data)
{
	uint32 off = (addr - region->start) + VDEV_RX_DATA_BODY_OFF;
	*data = *((uint16*)(&region->data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_udp_get_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 *data)
{
	uint32 off = (addr - region->start) + VDEV_RX_DATA_BODY_OFF;
	*data = *((uint32*)(&region->data[off]));
	return STD_E_OK;
}
static Std_ReturnType vdev_udp_put_data8(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 data)
{
	uint32 off = (addr - region->start) + VDEV_TX_DATA_BODY_OFF;
	*((uint8*)(&region->data[off])) = data;

	if (addr == VDEV_TX_FLAG(0)) {
		uint32 tx_off = VDEV_TX_DATA_BASE - region->start;
		Std_ReturnType err;
		memcpy(&vdev_control.comm.write_data.buffer[VDEV_TX_DATA_BODY_OFF], &region->data[tx_off + VDEV_TX_DATA_BODY_OFF], VDEV_TX_DATA_BODY_SIZE);
		memcpy(&vdev_control.comm.write_data.buffer[VDEV_TX_SIM_TIME(VDEV_SIM_INX_ME)],  (void*)&vdev_control.vdev_sim_time[VDEV_SIM_INX_ME], 8U);
		memcpy(&vdev_control.comm.write_data.buffer[VDEV_TX_SIM_TIME(VDEV_SIM_INX_YOU)], (void*)&vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU], 8U);
		//printf("sim_time=%llu\n", vdev_udp_control.vdev_sim_time[VDEV_SIM_INX_ME]);
		err = udp_comm_remote_write(&vdev_control.comm, vdev_control.remote_ipaddr);
		if (err != STD_E_OK) {
			printf("WARNING: vdevput_data8: udp send error=%d\n", err);
		}
	}
	else {
	}

	return STD_E_OK;
}
static Std_ReturnType vdev_udp_put_data16(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint16 data)
{
	uint32 off = (addr - region->start) + VDEV_TX_DATA_BODY_OFF;
	*((uint16*)(&region->data[off])) = data;
	return STD_E_OK;
}
static Std_ReturnType vdev_udp_put_data32(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint32 data)
{
	uint32 off = (addr - region->start) + VDEV_TX_DATA_BODY_OFF;
	*((uint32*)(&region->data[off])) = data;
	return STD_E_OK;
}
static Std_ReturnType vdev_udp_get_pointer(MpuAddressRegionType *region, CoreIdType core_id, uint32 addr, uint8 **data)
{
	uint32 off = (addr - region->start) + VDEV_TX_DATA_BODY_OFF;
	*data = &region->data[off];
	return STD_E_OK;
}
