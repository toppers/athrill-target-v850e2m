#include "device.h"
#include "cpuemu_ops.h"
#include "device_ex_serial_ops.h"
#include "concrete_executor/target/dbg_target_serial.h"
#include <stdio.h>
#include "std_device_ops.h"
#include "athrill_mpthread.h"
#include "vdev/vdev_private.h"
#ifdef SERIAL_FIFO_ENABLE
#include "serial_fifo.h"
#endif /* SERIAL_FIFO_ENABLE */

#ifdef CONFIG_STAT_PERF
ProfStatType cpuemu_dev_timer_prof;
ProfStatType cpuemu_dev_serial_prof;
ProfStatType cpuemu_dev_intr_prof;

#define CPUEMU_DEV_TIMER_PROF_START()	profstat_start(&cpuemu_dev_timer_prof)
#define CPUEMU_DEV_TIMER_PROF_END()		profstat_end(&cpuemu_dev_timer_prof)
#define CPUEMU_DEV_SERIAL_PROF_START()	profstat_start(&cpuemu_dev_serial_prof)
#define CPUEMU_DEV_SERIAL_PROF_END()		profstat_end(&cpuemu_dev_serial_prof)
#define CPUEMU_DEV_INTR_PROF_START()	profstat_start(&cpuemu_dev_intr_prof)
#define CPUEMU_DEV_INTR_PROF_END()		profstat_end(&cpuemu_dev_intr_prof)
#else
#define CPUEMU_DEV_TIMER_PROF_START()
#define CPUEMU_DEV_TIMER_PROF_END()
#define CPUEMU_DEV_SERIAL_PROF_START()
#define CPUEMU_DEV_SERIAL_PROF_END()
#define CPUEMU_DEV_INTR_PROF_START()
#define CPUEMU_DEV_INTR_PROF_END()
#endif /* CONFIG_STAT_PERF */

static DeviceExSerialOpType device_ex_serial_op = {
		.putchar = dbg_serial_putchar,
		.getchar = dbg_serial_getchar,
		.flush = NULL,
};
static DeviceExSerialOpType device_ex_serial_file_op = {
		.putchar = dbg_serial_putchar_file,
		.getchar = dbg_serial_getchar_file,
		.flush = dbg_serial_flush_file,
};

static void device_init_clock(MpuAddressRegionType *region)
{
	(void)mpthread_init();
	/*
	 * OSTC
	 */
	(void)device_io_write8(region, 0xFFFFF6C2, 0x01);

	/*
	 * ロック・レジスタ（ LOCKR）
	 */
	(void)device_io_write8(region, 0xFFFFF824, 0x00);

	return;
}

static uint32 enable_vdev = 0;
static void (*device_supply_clock_vdev_func) (DeviceClockType *) = NULL;

void device_init(CpuType *cpu, DeviceClockType *dev_clock)
{
	uint32 enable_mros = 0;
	char *path;
	dev_clock->clock = 0;
	dev_clock->intclock = 0;
	dev_clock->min_intr_interval = DEVICE_CLOCK_MAX_INTERVAL;
	dev_clock->can_skip_clock = FALSE;

	device_init_clock(&mpu_address_map.map[MPU_ADDRESS_REGION_INX_PH0]);
	device_init_intc(cpu, &mpu_address_map.map[MPU_ADDRESS_REGION_INX_INTC]);
	device_init_timer(&mpu_address_map.map[MPU_ADDRESS_REGION_INX_PH0]);

	device_init_serial(&mpu_address_map.map[MPU_ADDRESS_REGION_INX_SERIAL]);
	device_ex_serial_register_ops(0U, &device_ex_serial_op);
	if (cpuemu_get_devcfg_string("SERIAL_FILE_PATH", &path) == STD_E_OK) {
		device_ex_serial_register_ops(1U, &device_ex_serial_file_op);
	}
	else {
		device_ex_serial_register_ops(1U, &device_ex_serial_op);
	}
	cpuemu_get_devcfg_value("DEBUG_FUNC_ENABLE_MROS", &enable_mros);
	if (enable_mros != 0) {
		device_init_can(&mpu_address_map.map[MPU_ADDRESS_REGION_INX_CAN]);
	}
	cpuemu_get_devcfg_value("DEBUG_FUNC_ENABLE_VDEV", &enable_vdev);
	if (enable_vdev != 0) {
		char *sync_type;
		VdevIoOperationType op_type = VdevIoOperation_UDP;
		if (cpuemu_get_devcfg_string("DEBUG_FUNC_VDEV_SIMSYNC_TYPE", &sync_type) == STD_E_OK) {
			if (strncmp(sync_type, "MMAP", 4) == 0) {
				op_type = VdevIoOperation_MMAP;
				device_supply_clock_vdev_func = device_supply_clock_vdev_mmap;
			}
			else {
				device_supply_clock_vdev_func = device_supply_clock_vdev_udp;
			}
		}
		else {
			device_supply_clock_vdev_func = device_supply_clock_vdev_udp;
		}
		device_init_vdev(&mpu_address_map.map[MPU_ADDRESS_REGION_INX_VDEV], op_type);
	}
#ifdef SERIAL_FIFO_ENABLE
	athrill_device_init_serial_fifo();
#endif /* SERIAL_FIFO_ENABLE */
	return;
}

void device_supply_clock(DeviceClockType *dev_clock)
{
	dev_clock->min_intr_interval = DEVICE_CLOCK_MAX_INTERVAL;
	dev_clock->can_skip_clock = TRUE;

	CPUEMU_DEV_TIMER_PROF_START();
	device_supply_clock_timer(dev_clock);
	CPUEMU_DEV_TIMER_PROF_END();

	CPUEMU_DEV_SERIAL_PROF_START();
	device_supply_clock_serial(dev_clock);
	CPUEMU_DEV_SERIAL_PROF_END();

	if (enable_vdev == TRUE) {
		CPUEMU_DEV_INTR_PROF_START();
		device_supply_clock_vdev_func(dev_clock);
		CPUEMU_DEV_INTR_PROF_END();
	}

#ifdef SERIAL_FIFO_ENABLE
	CPUEMU_DEV_INTR_PROF_START();
	athrill_device_supply_clock_serial_fifo(dev_clock);
	CPUEMU_DEV_INTR_PROF_END();
#endif /* SERIAL_FIFO_ENABLE */

	CPUEMU_DEV_INTR_PROF_START();
	device_supply_clock_intc(dev_clock);
	CPUEMU_DEV_INTR_PROF_END();

	return;
}


int device_io_write8(MpuAddressRegionType *region,  uint32 addr, uint8 data)
{
	return region->ops->put_data8(region, CPU_CONFIG_CORE_ID_0, (addr & region->mask), data);
}
int device_io_write16(MpuAddressRegionType *region, uint32 addr, uint16 data)
{
	return region->ops->put_data16(region, CPU_CONFIG_CORE_ID_0, (addr & region->mask), data);
}

int device_io_write32(MpuAddressRegionType *region, uint32 addr, uint32 data)
{
	return region->ops->put_data32(region, CPU_CONFIG_CORE_ID_0, (addr & region->mask), data);
}

int device_io_read8(MpuAddressRegionType *region, uint32 addr, uint8 *data)
{
	return region->ops->get_data8(region, CPU_CONFIG_CORE_ID_0, (addr & region->mask), data);
}

int device_io_read16(MpuAddressRegionType *region, uint32 addr, uint16 *data)
{
	return region->ops->get_data16(region, CPU_CONFIG_CORE_ID_0, (addr & region->mask), data);
}

int device_io_read32(MpuAddressRegionType *region, uint32 addr, uint32 *data)
{
	return region->ops->get_data32(region, CPU_CONFIG_CORE_ID_0, (addr & region->mask), data);
}

void device_raise_int(uint16 intno)
{
	intc_raise_intr(intno);
}


static DevRegisterMappingType *search_map(uint32 table_num, DevRegisterMappingType *table, uint32 address, uint32 size)
{
	DevRegisterMappingType *map;
	int i;
	uint32 end_addr;


	for (i = 0; i < table_num; i++) {
		map = &table[i];
		end_addr = map->start_address + map->size;
		if (address < map->start_address) {
			continue;
		}
		else if (address >= end_addr) {
			continue;
		}
		return map;
	}

	return NULL;
}

void dev_register_mapping_write_data(uint32 coreId, uint32 table_num, DevRegisterMappingType *table, uint32 address, uint32 size)
{
	DevRegisterIoArgType arg;
	DevRegisterMappingType *map = search_map(table_num, table, address, size);
	if (map == NULL) {
		printf("ERROR:Device can not WRITE HIT address:0x%x %u\n", address, size);
		return;
	}
	arg.coreId = coreId;
	arg.address = address;
	arg.size = size;
	map->io(DevRegisterIo_Write, &arg);
	return;
}

void dev_register_mapping_read_data(uint32 coreId, uint32 table_num, DevRegisterMappingType *table, uint32 address, uint32 size)
{
	DevRegisterIoArgType arg;
	DevRegisterMappingType *map = search_map(table_num, table, address, size);
	if (map == NULL) {
		printf("ERROR:Device can not READ HIT address:0x%x %u\n", address, size);
		return;
	}
	arg.coreId = coreId;
	arg.address = address;
	arg.size = size;
	map->io(DevRegisterIo_Read, &arg);
	return;
}
