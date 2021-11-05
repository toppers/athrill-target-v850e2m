#include "op_exec_ops.h"
#include "cpu.h"
#include "bus.h"


/*
 * Format8
 */
int op_exec_tst1_8(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type8.reg1;
	sint32 disp16 = cpu->decoded_code->type8.disp;
	sint32 bit3 = cpu->decoded_code->type8.bit;
	uint32 addr;
	uint8 bit;
	Std_ReturnType err;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}

	addr = cpu->reg.r[reg1] + disp16;

	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}


	if ((bit & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
	}
	else {
		CPU_SET_Z(&cpu->reg);
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TST1 bit#3(%d), disp16(%d),r%d(0x%x):psw=0x%x\n", cpu->reg.pc, bit3, disp16, reg1, cpu->reg.r[reg1], sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW]));

	cpu->reg.pc += 4;

	return 0;
}
int op_exec_set1_8(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type8.reg1;
	sint32 disp16 = cpu->decoded_code->type8.disp;
	sint32 bit3 = cpu->decoded_code->type8.bit;
	uint32 addr;
	uint8 org_bit;
	uint8 bit;
	Std_ReturnType err;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}

	addr = cpu->reg.r[reg1] + disp16;

	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}


	org_bit = bit;

	bit |= (1 << bit3);
	err = bus_put_data8(cpu->core_id, addr, bit);
	if (err != STD_E_OK) {
		return -1;
	}

	if (((org_bit) & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
	}
	else {
		CPU_SET_Z(&cpu->reg);
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: SET1 bit#3(%d), disp16(%d), addr=0x%x r%d(0x%x):psw=0x%x, bit=0x%x\n", cpu->reg.pc, bit3, disp16, addr, reg1, cpu->reg.r[reg1], sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW], bit));

	cpu->reg.pc += 4;

	return 0;
}

int op_exec_clr1_8(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type8.reg1;
	sint32 disp16 = cpu->decoded_code->type8.disp;
	sint32 bit3 = cpu->decoded_code->type8.bit;
	uint32 addr;
	uint8 org_bit;
	uint8 bit;
	Std_ReturnType err;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}

	addr = cpu->reg.r[reg1] + disp16;
	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}

	org_bit = bit;

	bit &= ~(1 << bit3);
	err = bus_put_data8(cpu->core_id, addr, bit);
	if (err != STD_E_OK) {
		return -1;
	}

	if (((org_bit) & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
	}
	else {
		CPU_SET_Z(&cpu->reg);
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CLR1 bit#3(%d), disp16(%d), addr=0x%x r%d(0x%x):psw=0x%x, bit=0x%x\n", cpu->reg.pc, bit3, disp16, addr, reg1, cpu->reg.r[reg1], sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW], bit));

	cpu->reg.pc += 4;

	return 0;
}

int op_exec_not1_8(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type8.reg1;
	sint32 disp16 = cpu->decoded_code->type8.disp;
	sint32 bit3 = cpu->decoded_code->type8.bit;
	uint32 addr;
	uint8 org_bit;
	uint8 bit;
	Std_ReturnType err;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}

	addr = cpu->reg.r[reg1] + disp16;
	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}

	org_bit = bit;


	if (((org_bit) & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
		bit &= ~(1 << bit3);
	}
	else {
		CPU_SET_Z(&cpu->reg);
		bit |= (1 << bit3);
	}
	err = bus_put_data8(cpu->core_id, addr, bit);
	if (err != STD_E_OK) {
		return -1;
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(),
			"0x%x: NOT1 bit#3(%d), disp16(%d), addr=0x%x r%d(0x%x):psw=0x%x, bit=0x%x\n",
			cpu->reg.pc, bit3, disp16, addr, reg1, cpu->reg.r[reg1], sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW], bit));

	cpu->reg.pc += 4;

	return 0;
}


/*
 * Format9
 */
int op_exec_set1_9(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type9.gen;
	sint32 reg2 = cpu->decoded_code->type9.reg2;
	uint32 addr;
	uint8 org_bit;
	uint8 bit3;
	uint8 bit;
	Std_ReturnType err;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	/*
	 * （ 2） adr ← GR [reg1]
	 * Zフラグ ← Not (Load-memory-bit (adr, reg2) )
	 * Store-memory-bit (adr, reg2, 1)
	 */

	addr = cpu->reg.r[reg1];
	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}

	org_bit = bit;

	bit3 = (cpu->reg.r[reg2] & 0x07);
	bit |= (1 << bit3);
	err = bus_put_data8(cpu->core_id, addr, bit);
	if (err != STD_E_OK) {
		return -1;
	}

	if (((org_bit) & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
	}
	else {
		CPU_SET_Z(&cpu->reg);
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(),
			"0x%x: SET1 bit#3(%d), addr=0x%x r%d(0x%x) r%d(0x%x):psw=0x%x, bit=0x%x\n",
			cpu->reg.pc,
			bit3, addr, reg1, cpu->reg.r[reg1], reg2, cpu->reg.r[reg2],sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW], bit));

	cpu->reg.pc += 4;

	return 0;
}
int op_exec_clr1_9(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type9.gen;
	sint32 reg2 = cpu->decoded_code->type9.reg2;
	uint32 addr;
	uint8 bit;
	Std_ReturnType err;
	uint8 org_bit;
	uint8 bit3;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	/*
	 * （ 2） adr ← GR [reg1]
	 * Zフラグ ← Not (Load-memory-bit (adr, reg2) )
	 * Store-memory-bit (adr, reg2, 0)
	 */

	addr = cpu->reg.r[reg1];
	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}
	org_bit = bit;

	bit3 = (cpu->reg.r[reg2] & 0x07);
	bit &= ~(1 << bit3);
	err = bus_put_data8(cpu->core_id, addr, bit);
	if (err != STD_E_OK) {
		return -1;
	}
	if (((org_bit) & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
	}
	else {
		CPU_SET_Z(&cpu->reg);
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(),
			"0x%x: CLR1 bit#3(%d), addr=0x%x r%d(0x%x) r%d(0x%x):psw=0x%x, bit=0x%x\n",
			cpu->reg.pc,
			bit3, addr, reg1, cpu->reg.r[reg1], reg2, cpu->reg.r[reg2],sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW], bit));

	cpu->reg.pc += 4;

	return 0;
}

int op_exec_tst1_9(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type9.gen;
	sint32 reg2 = cpu->decoded_code->type9.reg2;
	uint32 addr;
	Std_ReturnType err;
	uint8 bit3;
	uint8 bit;


	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	addr = cpu->reg.r[reg1];

	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}

	bit3 = (cpu->reg.r[reg2] & 0x07);

	if ((bit & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
	}
	else {
		CPU_SET_Z(&cpu->reg);
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(),
			"0x%x: TST1 bit#3(%d), r%d(0x%x),r%d(0x%x):psw=0x%x\n",
			cpu->reg.pc, bit3, reg1, cpu->reg.r[reg1], reg2, cpu->reg.r[reg2], sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW]));

	cpu->reg.pc += 4;

	return 0;
}
int op_exec_not1_9(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type9.gen;
	sint32 reg2 = cpu->decoded_code->type9.reg2;
	uint32 addr;
	Std_ReturnType err;
	uint8 bit;
	uint8 org_bit;
	uint8 bit3;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	/*
	 * （ 2） adr ← GR [reg1]
	 * Zフラグ ← Not (Load-memory-bit (adr, reg2) )
	 * Store-memory-bit (adr, reg2, Zフラグ)
	 */
	addr = cpu->reg.r[reg1];
	err = bus_get_data8(cpu->core_id, addr, &bit);
	if (err != STD_E_OK) {
		return -1;
	}

	org_bit = bit;

	bit3 = (cpu->reg.r[reg2] & 0x07);

	if (((org_bit) & (1 << bit3)) == (1 << bit3)) {
		CPU_CLR_Z(&cpu->reg);
		bit &= ~(1 << bit3);
	}
	else {
		CPU_SET_Z(&cpu->reg);
		bit |= (1 << bit3);
	}
	err = bus_put_data8(cpu->core_id, addr, bit);
	if (err != STD_E_OK) {
		return -1;
	}

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(),
			"0x%x: NOT1 bit#3(%d), addr=0x%x r%d(0x%x) r%d(0x%x):psw=0x%x, bit=0x%x\n",
			cpu->reg.pc,
			bit3, addr, reg1, cpu->reg.r[reg1], reg2, cpu->reg.r[reg2],sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW], bit));

	cpu->reg.pc += 4;

	return 0;
}


static const uint32 schlr_search_bitval[2][32] = {
	{
		0, /* 0 */
		0, /* 1 */
		0, /* 2 */
		0, /* 3 */
		0, /* 4 */
		0, /* 5 */
		0, /* 6 */
		0, /* 7 */
		0, /* 8 */
		0, /* 9 */
		0, /* 10 */
		0, /* 11 */
		0, /* 12 */
		0, /* 13 */
		0, /* 14 */
		0, /* 15 */
		0, /* 16 */
		0, /* 17 */
		0, /* 18 */
		0, /* 19 */
		0, /* 20 */
		0, /* 21 */
		0, /* 22 */
		0, /* 23 */
		0, /* 24 */
		0, /* 25 */
		0, /* 26 */
		0, /* 27 */
		0, /* 28 */
		0, /* 29 */
		0, /* 30 */
		0, /* 31 */
	},
	{
		(1U << 0), /* 0 */
		(1U << 1), /* 1 */
		(1U << 2), /* 2 */
		(1U << 3), /* 3 */
		(1U << 4), /* 4 */
		(1U << 5), /* 5 */
		(1U << 6), /* 6 */
		(1U << 7), /* 7 */
		(1U << 8), /* 8 */
		(1U << 9), /* 9 */
		(1U << 10), /* 10 */
		(1U << 11), /* 11 */
		(1U << 12), /* 12 */
		(1U << 13), /* 13 */
		(1U << 14), /* 14 */
		(1U << 15), /* 15 */
		(1U << 16), /* 16 */
		(1U << 17), /* 17 */
		(1U << 18), /* 18 */
		(1U << 19), /* 19 */
		(1U << 20), /* 20 */
		(1U << 21), /* 21 */
		(1U << 22), /* 22 */
		(1U << 23), /* 23 */
		(1U << 24), /* 24 */
		(1U << 25), /* 25 */
		(1U << 26), /* 26 */
		(1U << 27), /* 27 */
		(1U << 28), /* 28 */
		(1U << 29), /* 29 */
		(1U << 30), /* 30 */
		(1U << 31), /* 31 */
	},
};

static int op_exec_schlr_9(TargetCoreType *cpu, uint32 search_bitval, bool search_left)
{
	sint32 reg2 = cpu->decoded_code->type9.reg2;
	sint32 reg3 = cpu->decoded_code->type9.rfu2;
	uint32 reg2_data = cpu->reg.r[reg2];
	uint32 i;
	uint32 count = 0;
	bool isFound = FALSE;
	uint32 result;
	char *dir = NULL;

	if (search_left == TRUE) {
		dir = "L";
		for (i = 0; i < 32; i++) {
			if ((reg2_data & (1U << (31 - i))) == schlr_search_bitval[search_bitval][(31 - i)]) {
				isFound = TRUE;
				break;
			}
			else {
				count++;
			}
		}
	}
	else {
		dir = "R";
		for (i = 0; i < 32; i++) {
			if ((reg2_data & (1U << (i))) == schlr_search_bitval[search_bitval][i]) {
				isFound = TRUE;
				break;
			}
			else {
				count++;
			}
		}
		DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(),
				"0x%x: SCH%dR r%d(0x%x) r%d(0x%x):%u\n",
				cpu->reg.pc,
				search_bitval,
				reg3, cpu->reg.r[reg2], reg3, cpu->reg.r[reg3],
				count + 1));
	}

	//CY 最後にビット（search_bitval）が見つかったとき 1，そうでないとき 0
	if (count == 31) {
		CPU_SET_CY(&cpu->reg);
	}
	else {
		CPU_SET_CY(&cpu->reg);
	}

	CPU_CLR_OV(&cpu->reg);
	CPU_CLR_S(&cpu->reg);

	//ビット（search_bitval）が見つからなかったとき 1，そうでないとき 0
	if (isFound == FALSE) {
		CPU_SET_Z(&cpu->reg);
		result = 0;
	}
	else {
		CPU_CLR_Z(&cpu->reg);
		result = count + 1;
	}
	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(),
			"0x%x: SCH%d%s r%d(0x%x) r%d(0x%x):psw=0x%x, %u\n",
			cpu->reg.pc,
			search_bitval,
			dir,
			reg2, cpu->reg.r[reg2], reg3, cpu->reg.r[reg3],
			sys_get_cpu_base(&cpu->reg)->r[SYS_REG_PSW], result));

	cpu->reg.r[reg3] = result;
	cpu->reg.pc += 4;

	return 0;
}

/*
 * Format9
 */
int op_exec_sch0l_9(TargetCoreType *cpu)
{
	return op_exec_schlr_9(cpu, 0, TRUE);
}
int op_exec_sch1l_9(TargetCoreType *cpu)
{
	return op_exec_schlr_9(cpu, 1, TRUE);
}
int op_exec_sch0r_9(TargetCoreType *cpu)
{
	return op_exec_schlr_9(cpu, 0, FALSE);
}
int op_exec_sch1r_9(TargetCoreType *cpu)
{
	return op_exec_schlr_9(cpu, 1, FALSE);
}
