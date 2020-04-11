#include "device.h"
#include "inc/vdev.h"
#include "vdev/vdev_private.h"
#include "std_errno.h"
#include "mpu_types.h"
#include "cpuemu_ops.h"
#include "std_cpu_ops.h"
#include "mpu_ops.h"
#include "assert.h"
#include <string.h>
#include <stdio.h>

MpuAddressRegionOperationType	vdev_memory_operation;
VdevControlType vdev_control;

void device_init_vdev(MpuAddressRegionType *region, VdevIoOperationType op_type)
{
	vdev_control.region = region;
	vdev_control.config.is_wait = TRUE;

	vdev_control.cpu_freq = DEFAULT_CPU_FREQ; /* 100MHz */
	(void)cpuemu_get_devcfg_value("DEVICE_CONFIG_TIMER_FD", &vdev_control.cpu_freq);

	if (op_type == VdevIoOperation_UDP) {
		vdev_memory_operation = vdev_udp_memory_operation;
		device_init_vdev_udp(region);
	}
	else if (op_type == VdevIoOperation_MMAP) {
		vdev_memory_operation = vdev_mmap_memory_operation;
		device_init_vdev_mmap(region);
	}
	else {
		ASSERT(0);
	}
	return;
}

#if 0
static uint64 unity_interval_vtime = 0; /* usec */
static struct timeval unity_notify_time = { 0, 0 };
static uint64 average_unity_notify_interval = 0;
static double average_unity_vtime_ratio = 0.0;
#define average_count	100

static uint64 vdev_get_unity_sim_time(DeviceClockType *dev_clock)
{
	if (vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU] == 0) {
		return 0;
	}
	if (cpu_is_halt_all() == FALSE) {
		return ( vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU] * ((uint64)vdev_control.cpu_freq) );
	}
#if 1
	struct timeval curr_time;
	struct timeval elaps;
	gettimeofday(&curr_time, NULL);

	/*
	 * adjust unity virtual time
	 */
	cpuemu_timeval_sub(&curr_time, &unity_notify_time, &elaps);
	uint64 real_time_elaps = (uint64)((double)cpuemu_timeval2usec(&elaps) * average_unity_vtime_ratio);
#if 0
	{
		static int count = 0;
		if (((count++) % 1000000) == 0) {
			printf("elaps=%llu\n", real_time_elaps);
		}
	}
#endif
	//return ( vdev_udp_control.vdev_sim_time[VDEV_SIM_INX_YOU] * ((uint64)vdev_udp_control.cpu_freq) );
	return ( ( vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU] + (real_time_elaps) )  * ((uint64)vdev_control.cpu_freq) );

#else
	return ( vdev_control.vdev_sim_time[VDEV_SIM_INX_YOU] * ((uint64)vdev_control.cpu_freq) );
#endif
}
#endif

#if 0
static void vdev_calc_predicted_virtual_time(uint64 prev_stime, uint64 curr_stime)
{
	static struct timeval prev_time = { 0, 0 };
	//static uint64 prev_athrill_virtual_time = 0;
	//uint64 current_athrill_virtual_time = vdev_udp_control.vdev_sim_time[VDEV_SIM_INX_ME];
	struct timeval elaps;
	cpuemu_timeval_sub(&unity_notify_time, &prev_time, &elaps);

	uint64 virtual_time_elaps = curr_stime - prev_stime;
	uint64 real_time_elaps = cpuemu_timeval2usec(&elaps);

	{
		static int count = 0;
		static uint64 virtual_time_elaps_sum = 0;
		static uint64 real_time_elaps_sum = 0;
		virtual_time_elaps_sum += virtual_time_elaps;
		real_time_elaps_sum += real_time_elaps;
		if (((++count) % average_count) == 0) {
			average_unity_notify_interval = real_time_elaps_sum/average_count;
			average_unity_vtime_ratio = (double)virtual_time_elaps_sum/(double)real_time_elaps_sum;
#if 0
			printf("virtual_elapls = %llu usec real_elapls = %llu rate=%lf athrill_elaps= %llu\n",
					virtual_time_elaps_sum/average_count, average_unity_notify_interval,
					average_unity_vtime_ratio,
					current_athrill_virtual_time - prev_athrill_virtual_time);
#endif
			virtual_time_elaps_sum = 0;
			real_time_elaps_sum = 0;
		}
	}
	prev_time = unity_notify_time;
	//prev_athrill_virtual_time = current_athrill_virtual_time;
}
#endif



