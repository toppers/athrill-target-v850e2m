#include "op_exec_ops.h"
#include "cpu_fpu_ops.h"
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#define _GNU_SOURCE
#include <fenv.h>
#include <float.h>

//#define DEBUG_FPU
#ifdef DEBUG_FPU
static void show_float_data(FloatBinaryDataType x)
{
	printf("float_data=%e\n", x.data);
	printf("31:=%u\n", GET_FLOAT_SIGN_BIT_VALUE(x));
	printf("30-23:=%d\n", GET_FLOAT_EXP_VALUE(x));
	printf("30-23-127:=%d\n", GET_FLOAT_EXPB_VALUE(x));
	unsigned int ma = GET_FLOAT_MANTISSA_VALUE(x);
	int i;
	for (i = FLOAT_MANTISSA_POS_START; i >= FLOAT_MANTISSA_POS_END; i--) {
		printf("%2d ", i);
	}
	printf("\n");
	for (i = FLOAT_MANTISSA_POS_START; i >= FLOAT_MANTISSA_POS_END; i--) {
		printf("%2d ", GET_FLOAT_MANTISSA_BIT_VALUE(ma, i));
	}
	printf("\n");

	if (FLOAT_IS_ZERO(x)) {
		printf("zero\n");
	}
	if (FLOAT_IS_NORMAL(x)) {
		printf("normal\n");
	}
	if (FLOAT_IS_SBNORM(x)) {
		printf("subnormal\n");
	}
	if (FLOAT_IS_INF(x)) {
		printf("inf\n");
	}
	if (FLOAT_IS_NAN(x)) {
		printf("nan\n");
	}
	if (FLOAT_IS_PLUS(x)) {
		printf("+\n");
	}
	if (FLOAT_IS_MINUS(x)) {
		printf("-\n");
	}
    return;
}
#endif

typedef enum {
    FpuRounding_RN = 0,
    FpuRounding_RU,
    FpuRounding_RD,
    FpuRounding_RZ,
} FpuConfigRoundingType;

typedef struct {
    bool is_FPSR_FS;
    FpuConfigRoundingType round_type;
} FpuConfigSettingType;

/*
 * FPSR.FS = 1 : TRUE
 * FPSR.FS = 0 : FALSE
 */
static void prepare_float_op(TargetCoreType *cpu, FloatExceptionType *exp, FpuConfigSettingType *cp)
{
    FLOAT_EXCEPTION_BIT_RESET(exp);
    cp->is_FPSR_FS = TRUE;

    SysFpuRmType cfg = sys_get_fpsr_rm(&cpu->reg);
    cp->round_type = FpuRounding_RN;
    switch (cfg) {
    case SysFpuRm_RN:
        cp->round_type = FpuRounding_RN;
        break;
    case SysFpuRm_RZ:
        cp->round_type = FpuRounding_RZ;
        break;
    case SysFpuRm_RP:
        cp->round_type = FpuRounding_RU;
        break;
    case SysFpuRm_RD:
        cp->round_type = FpuRounding_RD;
        break;
    default:
        break;
    }

    switch (cp->round_type) {
    case FpuRounding_RN:
        (void)fesetround(FE_TONEAREST);
        break;
    case FpuRounding_RU:
        (void)fesetround(FE_UPWARD);
        break;
    case FpuRounding_RD:
        (void)fesetround(FE_DOWNWARD);
        break;
    case FpuRounding_RZ:
        (void)fesetround(FE_TOWARDZERO);
        break;
    default:
        (void)fesetround(FE_TONEAREST);
        break;
    }
   	(void)feclearexcept(FE_ALL_EXCEPT);
    return;
}
static void end_float_op(TargetCoreType *cpu, FloatExceptionType *exp)
{
    if (fetestexcept(FE_ALL_EXCEPT) == 0) {
        return;
    }
    if (fetestexcept(FE_DIVBYZERO)) {
        FLOAT_EXCEPTION_INTR_BIT_SET(exp, FLOAT_EXCEPTION_BIT_Z);
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_Z);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_Z);
        //printf("FE_DIVBYZERO\n");
    }
    if (fetestexcept(FE_INEXACT)) {
        FLOAT_EXCEPTION_INTR_BIT_SET(exp, FLOAT_EXCEPTION_BIT_I);
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_I);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_I);
        //printf("FE_INEXACT\n");
    }
    if (fetestexcept(FE_INVALID)) {
        FLOAT_EXCEPTION_INTR_BIT_SET(exp, FLOAT_EXCEPTION_BIT_V);
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        //printf("FE_INVALID\n");
    }
    if (fetestexcept(FE_OVERFLOW)) {
        FLOAT_EXCEPTION_INTR_BIT_SET(exp, FLOAT_EXCEPTION_BIT_O);
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_O);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_O);
        //printf("FE_OVERFLOW\n");
    }
    if (fetestexcept(FE_UNDERFLOW)) {
        FLOAT_EXCEPTION_INTR_BIT_SET(exp, FLOAT_EXCEPTION_BIT_U);
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_U);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_U);
        //printf("FE_UNDERFLOW\n");
    }
    return;
}
static inline void set_subnormal_operand(TargetCoreType *cpu, FpuConfigSettingType *cp, FloatBinaryDataType *operand)
{
    if (!FLOAT_IS_SBNORM(*operand)) {
        return;
    }
    if (cp->is_FPSR_FS == FALSE) {
        // not supported.
        return;
    }
    // TODO FPSR レジスタの IF ビット
    if (FLOAT_IS_MINUS(*operand)) {
        operand->binary = 0;
        FLOAT_SIGN_BIT_SET(*operand);
    }
    else {
        operand->binary = 0;
    }
    return;
}

static inline void set_subnormal_result(TargetCoreType *cpu, FpuConfigSettingType *cp, FloatBinaryDataType *result)
{
    if (!FLOAT_IS_SBNORM(*result)) {
        return;
    }
    if (cp->is_FPSR_FS == FALSE) {
        // not supported.
        return;
    }
    //TODO FN = 1 未サポート
    switch (cp->round_type) {
    case FpuRounding_RN:
    case FpuRounding_RZ:
        if (FLOAT_IS_MINUS(*result)) {
            FLOAT_SIGN_BIT_SET(*result);
            result->binary = 0;
        }
        else {
            result->binary = 0;
        }
        break;
    case FpuRounding_RU:
        if (FLOAT_IS_MINUS(*result)) {
            result->binary = 0;
            FLOAT_SIGN_BIT_SET(*result);
        }
        else {
            result->data = FLT_MIN;
        }
        break;
    case FpuRounding_RD:
        if (FLOAT_IS_MINUS(*result)) {
            result->data = FLT_MIN;
            FLOAT_SIGN_BIT_SET(*result);
        }
        else {
            result->binary = 0;
        }
        break;
    default:
        break;
    }

    return;
}
static inline void set_subnormal_operand_double(TargetCoreType *cpu, FpuConfigSettingType *cp, DoubleBinaryDataType *operand)
{
    if (!DOUBLE_IS_SBNORM(*operand)) {
        return;
    }
    if (cp->is_FPSR_FS == FALSE) {
        // not supported.
        return;
    }
    // TODO FPSR レジスタの IF ビット
    if (DOUBLE_IS_MINUS(*operand)) {
        operand->binary[0] = 0;
        operand->binary[1] = 0;
        DOUBLE_SIGN_BIT_SET(*operand);
    }
    else {
        operand->binary[0] = 0;
        operand->binary[1] = 0;
    }
    return;
}

static inline void set_subnormal_result_double(TargetCoreType *cpu, FpuConfigSettingType *cp, DoubleBinaryDataType *result)
{
    if (!DOUBLE_IS_SBNORM(*result)) {
        return;
    }
    if (cp->is_FPSR_FS == FALSE) {
        // not supported.
        return;
    }
    //TODO FN = 1 未サポート
    switch (cp->round_type) {
    case FpuRounding_RN:
    case FpuRounding_RZ:
        if (DOUBLE_IS_MINUS(*result)) {
            DOUBLE_SIGN_BIT_SET(*result);
            result->binary[0] = 0;
            result->binary[1] = 0;
        }
        else {
            result->binary[0] = 0;
            result->binary[1] = 0;
        }
        break;
    case FpuRounding_RU:
        if (DOUBLE_IS_MINUS(*result)) {
            result->binary[0] = 0;
            result->binary[1] = 0;
            DOUBLE_SIGN_BIT_SET(*result);
        }
        else {
            result->data = DBL_MIN;
        }
        break;
    case FpuRounding_RD:
        if (DOUBLE_IS_MINUS(*result)) {
            result->data = DBL_MIN;
            DOUBLE_SIGN_BIT_SET(*result);
        }
        else {
            result->binary[0] = 0;
            result->binary[1] = 0;
        }
        break;
    default:
        break;
    }

    return;
}

int op_exec_absf_s_F(TargetCoreType *cpu)
{
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];
    op_absf_s(&reg2_data, &result_data);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: ABSF_S r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;

    /*
     * FPSR.FS = 1 でもサブノーマル数の入力はフラッシュされません。
     */

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_addf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = reg1_data.data + reg2_data.data;
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: ADDF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}

int op_exec_mulf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = reg1_data.data * reg2_data.data;
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MULF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_mulf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = reg1_data.data * reg2_data.data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MULF_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n",
        cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_addf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = reg1_data.data + reg2_data.data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: ADDF_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n",
        cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];


	cpu->reg.pc += 4;
    return 0;
}

int op_exec_divf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = reg2_data.data / reg1_data.data;
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: DIVF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_subf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = reg2_data.data - reg1_data.data;
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: SUBF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}


int op_exec_maxf_s_F(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    if (float_is_snan(reg1_data) || float_is_snan(reg2_data)) {
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        float_get_qnan(&result_data);
    }
    else {
        if (reg1_data.data > reg2_data.data) {
            result_data.data = reg1_data.data;
        }
        else {
            result_data.data = reg2_data.data;
        }
    }

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MAX_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_minf_s_F(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    if (float_is_snan(reg1_data) || float_is_snan(reg2_data)) {
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        float_get_qnan(&result_data);
    }
    else {
        if (reg1_data.data < reg2_data.data) {
            result_data.data = reg1_data.data;
        }
        else {
            result_data.data = reg2_data.data;
        }
    }

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MIN_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_cmovf_s_F(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    uint32 fcbit = ( (cpu->decoded_code->type_f.subopcode & 0x0000000F) >> 1U );
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;
    uint8 data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    data = sys_get_fpsr_cc(&cpu->reg);
    if ((data & (1U << (uint8)fcbit)) != 0) {
        result_data.data = reg1_data.data;
    }
    else {
        result_data.data = reg2_data.data;
    }

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CMOVF_S fcbit(%u) r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, fcbit, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));

	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_negf_s_F(TargetCoreType *cpu)
{
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    result_data.data = - reg2_data.data;

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: NEGF_S r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_trfsr_F(TargetCoreType *cpu)
{
    uint32 fcbit = ( (cpu->decoded_code->type_f.subopcode & 0x0000000F) >> 1U );
    uint8 data;

    data = sys_get_fpsr_cc(&cpu->reg);

    if ((data & (1U << (uint8)fcbit)) != 0) {
        CPU_SET_Z(&cpu->reg);
    }
    else {
        CPU_CLR_Z(&cpu->reg);
    }

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRFSR_S fcbit(%d)\n", 
        cpu->reg.pc, fcbit));

	cpu->reg.pc += 4;
    return 0;
}

typedef enum {
    FpucCmpFcond_F = 0,
    FpucCmpFcond_UN,
    FpucCmpFcond_EQ,
    FpucCmpFcond_UEQ,
    FpucCmpFcond_OLT,
    FpucCmpFcond_ULT,
    FpucCmpFcond_OLE,
    FpucCmpFcond_ULE,
    FpucCmpFcond_SF,
    FpucCmpFcond_NGLE,
    FpucCmpFcond_SEQ,
    FpucCmpFcond_NGL,
    FpucCmpFcond_LT,
    FpucCmpFcond_NGE,
    FpucCmpFcond_LE,
    FpucCmpFcond_NGT,
} FpuCmpFcondType;

int op_exec_cmpf_s_F(TargetCoreType *cpu)
{
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 fcond = cpu->decoded_code->type_f.reg3;
    uint32 fcbit = ( (cpu->decoded_code->type_f.subopcode & 0x0000000F) >> 1U );
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    bool result_less;
    bool result_equal;
    bool result_unordered;
    uint32 result_data;
    uint8 data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];

    /*
    * if isNaN(reg1) or isNaN(reg2) then
    * result.less ← 0
    * result.equal ← 0
    * result.unordered ← 1
    * if fcond[3] == 1 then
    * 無効演算例外を検出
    * endif
    * else
    * result.less ← reg2 < reg1
    * result.equal ← reg2 == reg1
    * result.unordered ← 0
    * endif
    * FPSR.CCn ← (fcond[2] & result.less) | (fcond[1] & result.equal) |
    * (fcond[0] & result.unordered)
    */
    if (FLOAT_IS_NAN(reg1_data) || FLOAT_IS_NAN(reg2_data)) {
        result_less = FALSE;
        result_equal = FALSE;
        result_unordered = TRUE;
        if ((fcond & 0x00000008) != 0) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
    }
    else {
        result_less = (reg2_data.data < reg1_data.data);
        result_equal = (reg2_data.data == reg1_data.data);
        result_unordered = FALSE;
    }
    result_data =   (result_less << 2U) |
                    (result_equal << 1U) |
                    (result_unordered << 0U);
    data = sys_get_fpsr_cc(&cpu->reg);
    if ((result_data & (fcond & 0x00000007)) != 0) {
        data |= (1U << (uint8)fcbit);
    }
    else {
        data &= ~(1U << (uint8)fcbit);
    }
    sys_set_fpsr_cc(&cpu->reg,  data);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CMPF_S r%d(%f),r%d(%f),fcond(0x%x), fcbit(%d):0x%x\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, fcond, fcbit, data));

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_fmaf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = fmaf(reg2_data.data, reg1_data.data, reg3_data.data);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: FMAF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_fmsf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = fmaf(reg2_data.data, reg1_data.data, -reg3_data.data);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: FMSF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_fnmaf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = fmaf(reg2_data.data, reg1_data.data, reg3_data.data);
        FLOAT_SET_SIGN_REVERSE(result_data);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: FNMAF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_fnmsf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = fmaf(reg2_data.data, reg1_data.data, -reg3_data.data);
        FLOAT_SET_SIGN_REVERSE(result_data);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: FNMSF_S r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_ws_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        result_data.data = (float32)((sint32)reg2_data.binary);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_WS r%d(%u),r%d(%f):%f\n", 
        cpu->reg.pc, reg2, reg2_data.binary, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_ds_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    DoubleBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2_0 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = (float32)(reg2_data.data);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_DS r%d(%lf),r%d(%f):%f\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_sd_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg3_data;
    FloatBinaryDataType reg2_data;
    DoubleBinaryDataType result_data;

	if (reg3_0 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];
	reg2_data.binary = cpu->reg.r[reg2];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        result_data.data = (float64)(reg2_data.data);
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_SD r%d(%f),r%d(%lf):%lf\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_ls_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_0_data;
    FloatBinaryDataType reg2_1_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2_0 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_0_data.binary = cpu->reg.r[reg2_0];
	reg2_1_data.binary = cpu->reg.r[reg2_1];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        sint64 result;
        sint32 *result_array = (sint32*)&result;
        result_array[0] = (sint32)reg2_0_data.binary;
        result_array[1] = (sint32)reg2_1_data.binary;

        result_data.data = (float32)(result);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_LS r%d(%d),r%d(%d),r%d(%f):%f\n", 
        cpu->reg.pc, reg2_0, (sint32)reg2_0_data.binary, reg2_1, (sint32)reg2_1_data.binary, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_uls_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_0_data;
    FloatBinaryDataType reg2_1_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2_0 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_0_data.binary = cpu->reg.r[reg2_0];
	reg2_1_data.binary = cpu->reg.r[reg2_1];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        uint64 result;
        uint32 *result_array = (uint32*)&result;
        result_array[0] = reg2_0_data.binary;
        result_array[1] = reg2_1_data.binary;

        result_data.data = (float32)(result);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_ULS r%d(%f),r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg2_0, reg2_0_data.data, reg2_1, reg2_1_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}

int op_exec_cvtf_suw_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        uint64 result;
        bool is_invalid = FALSE;
        if (FLOAT_IS_NAN(reg2_data) || FLOAT_IS_INF(reg2_data)) {
            if (FLOAT_IS_PLUS(reg2_data)) {
                result = (uint64)UINT_MAX;
            }
            else {
                result = 0;
            }
            is_invalid = TRUE;
        }
        else {
            result = (uint64)reg2_data.data;
            if (result > ((uint64)UINT_MAX)) {
                result = (uint64)UINT_MAX;
                is_invalid = TRUE;
            }
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data.binary = (uint32)result;

    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_SUW r%d(%f),r%d(%f):%u\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, (uint32)result_data.binary));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_cvtf_sw_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        sint64 result;
        bool is_invalid = FALSE;
        if (FLOAT_IS_NAN(reg2_data) || FLOAT_IS_INF(reg2_data)) {
            if (FLOAT_IS_PLUS(reg2_data)) {
                result = (sint64)INT_MAX;
            }
            else {
                result = (sint64)( -((sint32)INT_MAX) );
            }
            is_invalid = TRUE;
        }
        else {
            result = (sint64)reg2_data.data;
            if ( result < ((sint64)( -((sint32)INT_MAX))) ) {
                result = (sint64)( -((sint32)INT_MAX) );
                is_invalid = TRUE;
            } 
            else if (result > ((sint64)INT_MAX)) {
                result = (sint64)INT_MAX;
                is_invalid = TRUE;
            }
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data.binary = (sint32)result;

    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_SW r%d(%f),r%d(%f):%d\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, (sint32)result_data.binary));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_uws_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        result_data.data = (float32)((uint32)reg2_data.binary);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF_UWS r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
	cpu->reg.r[reg3] = result_data.binary;


	cpu->reg.pc += 4;
    return 0;
}

int op_exec_trncf_sw_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        sint64 result;
        bool is_invalid = FALSE;
        if (FLOAT_IS_NAN(reg2_data) || FLOAT_IS_INF(reg2_data)) {
            if (FLOAT_IS_PLUS(reg2_data)) {
                result = (sint64)INT_MAX;
            }
            else {
                result = (sint64)( -((sint32)INT_MAX) );
            }
            is_invalid = TRUE;
        }
        else {
            result = (sint64)truncf(reg2_data.data);
            if ( result < ((sint64)( -((sint32)INT_MAX))) ) {
                result = (sint64)( -((sint32)INT_MAX) );
                is_invalid = TRUE;
            } 
            else if (result > ((sint64)INT_MAX)) {
                result = (sint64)INT_MAX;
                is_invalid = TRUE;
            }
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data.binary = (sint32)result;

    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF_SW r%d(%f),r%d(%f):%d\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, (sint32)result_data.binary));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}

int op_exec_trncf_suw_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        uint64 result;
        bool is_invalid = FALSE;
        if (FLOAT_IS_NAN(reg2_data) || FLOAT_IS_INF(reg2_data)) {
            if (FLOAT_IS_PLUS(reg2_data)) {
                result = (uint64)UINT_MAX;
            }
            else {
                result = 0;
            }
            is_invalid = TRUE;
        }
        else {
            result = (uint64)truncf(reg2_data.data);
            if (result > ((uint64)UINT_MAX)) {
                result = (uint64)UINT_MAX;
                is_invalid = TRUE;
            }
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data.binary = (uint32)result;

    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF_SUW r%d(%f),r%d(%f):%u\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, (uint32)result_data.binary));
	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;
}



void fpu_sync_sysreg(TargetCoreType *cpu, uint32 regid, uint32 selid)
{
    if (selid != SYS_GRP_FPU) {
        return;
    }

    switch (regid) {
    case SYS_REG_FPST:
    {
        uint8 data = sys_get_fpst_xc(&cpu->reg);
        sys_set_fpsr_xc(&cpu->reg, data);
        data = sys_get_fpst_xp(&cpu->reg);
        sys_set_fpsr_xp(&cpu->reg, data);
        if (sys_isset_fpst_if(&cpu->reg)) {
            sys_set_fpsr_if(&cpu->reg);
        }
        else {
            sys_clr_fpsr_if(&cpu->reg);
        }
        break;
    }
    case SYS_REG_FPCC:
    {
        uint8 data = sys_get_fpcc_cc(&cpu->reg);
        sys_set_fpsr_cc(&cpu->reg, data);
        break;
    }
    case SYS_REG_FPCFG:
    {
        uint8 data = sys_get_fpcfg_rm(&cpu->reg);
        sys_set_fpsr_rm(&cpu->reg, data);
        data = sys_get_fpcfg_xe(&cpu->reg);
        sys_set_fpsr_xe(&cpu->reg, data);
        break;
    }
    case SYS_REG_FPEC:
    case SYS_REG_FPEPC:
    case SYS_REG_FPSR:
    default:
        break;
    }
    return;
}

int op_exec_maddf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;
    uint32 subopcode = cpu->decoded_code->type_f.subopcode;
    /* detect reg4 */
    uint32 reg4 = ((subopcode & 0xe) | ((subopcode >> 7) & 1));

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
    if ( reg4 >= CPU_GREG_NUM ) {
        return -1;
    }
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    // TODO: Full NaN/finite operation
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        set_subnormal_operand(cpu, &fpu_config, &reg3_data);
        result_data.data = reg2_data.data * reg1_data.data + reg3_data.data;
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MADD.S_F r%d(%f),r%d(%f),r%d(%f) r%d:%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, reg4, result_data.data));
	cpu->reg.r[reg4] = result_data.binary;

//    printf( "0x%x: MADD.S_F r%d(%f),r%d(%f),r%d(%f) r%d:%f\n",  cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, reg4, result_data.data);

	cpu->reg.pc += 4;
    
    return 0;
}
int op_exec_msubf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;
    uint32 subopcode = cpu->decoded_code->type_f.subopcode;
    /* detect reg4 */
    uint32 reg4 = ((subopcode & 0xe) | ((subopcode >> 7) & 1));

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
    if ( reg4 >= CPU_GREG_NUM ) {
        return -1;
    }
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    // TODO: Full NaN/finite operation
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        set_subnormal_operand(cpu, &fpu_config, &reg3_data);
        result_data.data = reg2_data.data * reg1_data.data - reg3_data.data;
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MSUB.S_F r%d(%f),r%d(%f),r%d(%f) r%d:%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, reg4, result_data.data));
	cpu->reg.r[reg4] = result_data.binary;

	cpu->reg.pc += 4;
    
    return 0;
}
int op_exec_nmaddf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;
    uint32 subopcode = cpu->decoded_code->type_f.subopcode;
    /* detect reg4 */
    uint32 reg4 = ((subopcode & 0xe) | ((subopcode >> 7) & 1));

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
    if ( reg4 >= CPU_GREG_NUM ) {
        return -1;
    }
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    // TODO: Full NaN/finite operation
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        set_subnormal_operand(cpu, &fpu_config, &reg3_data);
        result_data.data = -(reg2_data.data * reg1_data.data + reg3_data.data);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: NMADD.S_F r%d(%f),r%d(%f),r%d(%f) r%d:%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, reg4, result_data.data));

	cpu->reg.r[reg4] = result_data.binary;

	cpu->reg.pc += 4;
    
    return 0;

}
int op_exec_nmsubf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1 = cpu->decoded_code->type_f.reg1;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg1_data;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;
    uint32 subopcode = cpu->decoded_code->type_f.subopcode;
    /* detect reg4 */
    uint32 reg4 = ((subopcode & 0xe) | ((subopcode >> 7) & 1));

	if (reg1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
    if ( reg4 >= CPU_GREG_NUM ) {
        return -1;
    }
	reg1_data.binary = cpu->reg.r[reg1];
	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    // TODO: Full NaN/finite operation
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand(cpu, &fpu_config, &reg2_data);
        set_subnormal_operand(cpu, &fpu_config, &reg3_data);
        result_data.data = -(reg2_data.data * reg1_data.data - reg3_data.data);
        set_subnormal_result(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MSUB.S_F r%d(%f),r%d(%f),r%d(%f) r%d:%f\n", 
        cpu->reg.pc, reg1, reg1_data.data, reg2, reg2_data.data, reg3, reg3_data.data, reg4, result_data.data));
	cpu->reg.r[reg4] = result_data.binary;
	cpu->reg.pc += 4;
    
    return 0;

}

int op_exec_absf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = reg2_data.data;
        DOUBLE_SIGN_BIT_CLR(result_data);
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);
	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: ABSF_D r%d(%lf) : r%d :%lf\n",
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data.data));
//	printf( "0x%x: ABSF_D r%d(%lf) r%d(%lf) :%lf\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data,result_data.data);
	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];
	cpu->reg.pc += 4;

	return 0;
}
int op_exec_ceilf_dl_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    sint64 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
    // TODO:Overflow
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (sint64)reg2_data.data;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CEILF.DL r%d(%lf) : r%d :%lld\n",
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data));
//	printf( "0x%x: CEILF.DL r%d(%lf) r%d :%lld\n",cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data);
	cpu->reg.r[reg3_0] = (uint32)(result_data>>32);
	cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;

	return 0;
}
int op_exec_ceilf_dul_F(TargetCoreType *cpu)
{
    // double float -> unsigned 64
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    uint64 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {

        uint64 result;
        bool is_invalid = TRUE;
        if (DOUBLE_IS_NAN(reg2_data) || DOUBLE_IS_INF(reg2_data)) {
            if (DOUBLE_IS_PLUS(reg2_data) && DOUBLE_IS_INF(reg2_data)) {
                result = (uint64)ULONG_MAX;
            }
            else {
                result = (uint64)0;
            }
        } else if ( trunc(reg2_data.data) < 0 ) {
            result = (uint64)0;
        } else if ( trunc(reg2_data.data) > (uint64)ULONG_MAX) {
            result = (uint64)ULONG_MAX;
        } else {
            result = trunc(reg2_data.data);
            is_invalid = FALSE;
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data  = (uint64)result;

    }
    end_float_op(cpu, &ex);


	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CEILF.DUL r%d(%lf) : r%d :%llu\n",
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data));
//	printf( "0x%x: CEILF.DUL r%d(%lf) r%d :%llu\n",  cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data);
	cpu->reg.r[reg3_0] = (uint32)(result_data>>32);
	cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;

	return 0;
}

int op_exec_ceilf_duw_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
   	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    DoubleBinaryDataType reg2_data;
    sint32 result_data;
    sint32 reg3_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
    if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (sint32)trunc(reg2_data.data);
    }

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CEILF.DW r%d(%lf),r%d(%d):%d\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3, reg3_data, result_data));
//	printf("0x%x: CEILF.DW r%d(%lf),r%d(%d):%d\n",     cpu->reg.pc, reg2_0, reg2_data.data, reg3, reg3_data, result_data);

    cpu->reg.r[reg3] = result_data;
	cpu->reg.pc += 4;
    return 0;

}
int op_exec_ceilf_dw_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_cmovf_d_F(TargetCoreType *cpu)
{
    uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    uint32 fcbit = ( (cpu->decoded_code->type_f.subopcode & 0x0000000F) >> 1U );
    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;
    uint8 data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    data = sys_get_fpsr_cc(&cpu->reg);
    if ((data & (1U << (uint8)fcbit)) != 0) {
        result_data.data = reg1_data.data;
    }
    else {
        result_data.data = reg2_data.data;
    }

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CMOVF_D fcbit(%u) r%d(%lf),r%d(%lf),r%d(%lf):%lf\n", 
        cpu->reg.pc, fcbit, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//    printf("0x%x: CMOVF_D fcbit(%u) r%d(%lf),r%d(%lf),r%d(%lf):%lf\n",  cpu->reg.pc, fcbit, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);
	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];


	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cmpf_d_F(TargetCoreType *cpu)
{
	uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 fcond = cpu->decoded_code->type_f.reg3;
    uint32 fcbit = ( (cpu->decoded_code->type_f.subopcode & 0x0000000F) >> 1U );
    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    bool result_less;
    bool result_equal;
    bool result_unordered;
    uint32 result_data;
    uint8 data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}

  	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];

    /*
    * if isNaN(reg1) or isNaN(reg2) then
    * result.less ← 0
    * result.equal ← 0
    * result.unordered ← 1
    * if fcond[3] == 1 then
    * 無効演算例外を検出
    * endif
    * else
    * result.less ← reg2 < reg1
    * result.equal ← reg2 == reg1
    * result.unordered ← 0
    * endif
    * FPSR.CCn ← (fcond[2] & result.less) | (fcond[1] & result.equal) |
    * (fcond[0] & result.unordered)
    */
    if (DOUBLE_IS_NAN(reg1_data) || DOUBLE_IS_NAN(reg2_data)) {
        result_less = FALSE;
        result_equal = FALSE;
        result_unordered = TRUE;
        if ((fcond & 0x00000008) != 0) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
    }
    else {
        result_less = (reg2_data.data < reg1_data.data);
        result_equal = (reg2_data.data == reg1_data.data);
        result_unordered = FALSE;
    }
    result_data =   (result_less << 2U) |
                    (result_equal << 1U) |
                    (result_unordered << 0U);
    data = sys_get_fpsr_cc(&cpu->reg);
    if ((result_data & (fcond & 0x00000007)) != 0) {
        data |= (1U << (uint8)fcbit);
    }
    else {
        data &= ~(1U << (uint8)fcbit);
    }
    sys_set_fpsr_cc(&cpu->reg,  data);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CMPF_D r%d(%lf),r%d(%lf),fcond(0x%x), fcbit(%d):0x%x\n", 
        cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, fcond, fcbit, data));
//	printf("0x%x: CMPF_D r%d(%lf),r%d(%lf),fcond(0x%x), fcbit(%d):0x%x\n",  cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, fcond, fcbit, data);

	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_dl_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    sint64 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];


    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (sint64)reg2_data.data;
    }
    end_float_op(cpu, &ex);   

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.DL r%d(%lf),r%d:%lld\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, result_data));
//    printf( "0x%x: CVTF.DL r%d(%lf),r%d:%lld\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, result_data);

    cpu->reg.r[reg3_0] = (uint32)(result_data >> 32);
    cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;
    return 0;

}
int op_exec_cvtf_dul_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    uint64 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];


    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (uint64)reg2_data.data;
    }
    end_float_op(cpu, &ex);   

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.DUL r%d(%lf),r%d:%llu\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, result_data));
//	printf("0x%x: CVTF.DUL r%d(%lf),r%d:%llu\n",  cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, result_data);

    cpu->reg.r[reg3_0] = (uint32)(result_data >> 32);
    cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;
    return 0;
}

int op_exec_cvtf_duw_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3   = cpu->decoded_code->type_f.reg3;
    DoubleBinaryDataType reg2_data;
    uint32 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];

    // TODO:Overflow / Nan
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (uint32)reg2_data.data;
    }
    end_float_op(cpu, &ex);   

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.DUW r%d(%lf),r%d:%u\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3, result_data));
//    printf("0x%x: CVTF.DUW r%d(%lf),r%d:%u\n",  cpu->reg.pc, reg2_0, reg2_data.data, reg3, result_data);

    cpu->reg.r[reg3] = (uint32)result_data;
	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_dw_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3   = cpu->decoded_code->type_f.reg3;
    DoubleBinaryDataType reg2_data;
    sint32 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (sint32)reg2_data.data;
    }
    end_float_op(cpu, &ex);   

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.DUW r%d(%lf),r%d:%d\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3, result_data));
//    printf("0x%x: CVTF.DUW r%d(%lf),r%d:%d\n",  cpu->reg.pc, reg2_0, reg2_data.data, reg3, result_data);

    cpu->reg.r[reg3] = (sint32)result_data;
	cpu->reg.pc += 4;
    return 0;

}
int op_exec_cvtf_ld_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    sint64 reg2_data;
    DoubleBinaryDataType result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data = (uint64)((uint64)(cpu->reg.r[reg2_0])<<32 | cpu->reg.r[reg2_1]);

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        result_data.data = (double)reg2_data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);   

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.LD r%d(%lld),r%d:%lf\n", 
        cpu->reg.pc, reg2_0, reg2_data, reg3_0, result_data.data));
//    printf("0x%x: CVTF.LD r%d(%lld),r%d:%lf\n", cpu->reg.pc, reg2_0, reg2_data, reg3_0, result_data.data);

    cpu->reg.r[reg3_0] = (uint32)result_data.binary[0];
    cpu->reg.r[reg3_1] = (uint32)result_data.binary[1];

	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_uld_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    uint64 reg2_data;
    DoubleBinaryDataType result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data = (uint64)((uint64)(cpu->reg.r[reg2_0])<<32 | cpu->reg.r[reg2_1]);

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        result_data.data = (double)reg2_data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);   

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.ULD r%d(%llu),r%d:%lf\n", 
        cpu->reg.pc, reg2_0, reg2_data, reg3_0, result_data.data));
//    printf("0x%x: CVTF.ULD r%d(%llu),r%d:%lf\n", cpu->reg.pc, reg2_0, reg2_data, reg3_0, result_data.data);

    cpu->reg.r[reg3_0] = (uint32)result_data.binary[0];
    cpu->reg.r[reg3_1] = (uint32)result_data.binary[1];

	cpu->reg.pc += 4;
    return 0;

}
int op_exec_cvtf_uwd_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2   = cpu->decoded_code->type_f.reg2;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    uint32  reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data          = (uint32)cpu->reg.r[reg2];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        result_data.data = (double)reg2_data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.UWD r%d(%u),r%d(%lf):%lf\n",
        cpu->reg.pc, reg2,reg2_data, reg3_0, reg3_data.data,result_data.data));
//	printf("0x%x: CVTF.UWD r%d(%u),r%d(%lf):%lf\n", cpu->reg.pc, reg2,reg2_data, reg3_0, reg3_data.data, result_data.data);
	cpu->reg.r[reg3_0] = (uint32)result_data.binary[0];
	cpu->reg.r[reg3_1] = (uint32)result_data.binary[1];
	cpu->reg.pc += 4;
    return 0;
}
int op_exec_cvtf_wd_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2   = cpu->decoded_code->type_f.reg2;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    sint32  reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data          = cpu->reg.r[reg2];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        result_data.data = (double)reg2_data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: CVTF.WD r%d(%d),r%d(%lf):%lf\n",
        cpu->reg.pc, reg2,reg2_data, reg3_0, reg3_data.data,result_data.data));
//	printf("0x%x: CVTF.WD r%d(%d),r%d(%lf):%lf\n", cpu->reg.pc, reg2,reg2_data, reg3_0, reg3_data.data, result_data.data);
	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];
	cpu->reg.pc += 4;
    return 0;
}
int op_exec_divf_d_F(TargetCoreType *cpu)
{
        FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = reg2_data.data / reg1_data.data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: DIVF_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n",
        cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//	printf("0x%x: DIVF_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n", cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);

	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];

	cpu->reg.pc += 4;
    return 0;
}
int op_exec_floorf_dl_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_floorf_dul_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_floorf_duw_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_floorf_dw_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_maxf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;

    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType result_data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
 
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        DoubleBinaryDataType result;
        bool is_invalid = FALSE;
        if ( double_is_snan(reg1_data) || double_is_snan(reg2_data)){
            double_get_qnan(&result);
            is_invalid = TRUE;
        } else if ( double_is_qnan(reg1_data) || double_is_qnan(reg2_data)) {
            double_get_qnan(&result);
        } else {
            if ( reg1_data.data > reg2_data.data ) {
                result = reg1_data;
            } else {
                result = reg2_data;
            }
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data = result;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MAXF.D r%d(%lf),r%d(%ff) r%d:%lf\n", 
        cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, result_data.data));
//	printf( "0x%x: MAXF.D r%d(%lf),r%d(%ff) r%d:%lf\n", cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, result_data.data);

	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];
    cpu->reg.pc += 4;
    return 0;

	
}
int op_exec_minf_d_F(TargetCoreType *cpu)
{
	uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    if (double_is_snan(reg1_data) || double_is_snan(reg2_data)) {
        sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
        sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        double_get_qnan(&result_data);
    }
    else {
        if (reg1_data.data < reg2_data.data) {
            result_data.data = reg1_data.data;
        }
        else {
            result_data.data = reg2_data.data;
        }
    }

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: MIN_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n", 
        cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//	printf("0x%x: MIN_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n", cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);

	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];

	cpu->reg.pc += 4;
    return 0;
}
int op_exec_negf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data; 
    DoubleBinaryDataType reg3_data; 
    DoubleBinaryDataType result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        if ( double_is_qnan(reg2_data) ) {
            double_get_qnan(&result_data);
        } else if ( double_is_snan(reg2_data) ) {
            double_get_snan(&result_data);
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        } else {
            result_data = reg2_data;
            DOUBLE_SET_SIGN_REVERSE(result_data);
        }
    }
    end_float_op(cpu, &ex);
    DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: NEGF.D r%d(%lf),r%d(%lf):%lf\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//    printf("0x%x: NEGF.D r%d(%lf),r%d(%lf):%lf\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);
    
    cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];
	cpu->reg.pc += 4;
    return 0;
}
int op_exec_recipf_s_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3 = cpu->decoded_code->type_f.reg3;
    FloatBinaryDataType reg2_data;
    FloatBinaryDataType reg3_data;
    FloatBinaryDataType result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}

	reg2_data.binary = cpu->reg.r[reg2];
	reg3_data.binary = cpu->reg.r[reg3];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        if ( float_is_qnan(reg2_data)) {
            float_get_qnan(&result_data);
        } else if ( float_is_snan(reg2_data)) {
            float_get_snan(&result_data);
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        } else if ( FLOAT_IS_INF(reg2_data) ) {
            result_data.data = 0;
            if ( FLOAT_IS_MINUS(reg2_data)){
                // returns minus 0
                FLOAT_SIGN_BIT_SET(reg2_data);
            }
        } else if ( FLOAT_IS_ZERO(reg2_data) ) {
            result_data.data = INFINITY;
            if ( FLOAT_IS_MINUS(reg2_data)){
                // returns minus inifite
                FLOAT_SIGN_BIT_SET(reg2_data);
            }
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_Z);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_Z);
        } else {
            result_data.data = (float)1/reg2_data.data;
        }
    }
    end_float_op(cpu, &ex);
	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: RECIPF.S r%d(%f),r%d(%f):%f\n", 
        cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data));
//	printf("0x%x: RECIPF.S r%d(%f),r%d(%f):%f\n", cpu->reg.pc, reg2, reg2_data.data, reg3, reg3_data.data, result_data.data);

	cpu->reg.r[reg3] = result_data.binary;

	cpu->reg.pc += 4;
    return 0;

}

int op_exec_recipf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
    uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}

	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        if ( double_is_qnan(reg2_data)) {
            double_get_qnan(&result_data);
        } else if ( double_is_snan(reg2_data)) {
            double_get_snan(&result_data);
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        } else if ( DOUBLE_IS_INF(reg2_data) ) {
            result_data.data = 0;
            if ( DOUBLE_IS_MINUS(reg2_data)){
                // returns minus 0
                DOUBLE_SIGN_BIT_SET(reg2_data);
            }
        } else if ( DOUBLE_IS_ZERO(reg2_data) ) {
            result_data.data = (double)INFINITY;
            if ( DOUBLE_IS_MINUS(reg2_data)){
                // returns minus inifite
                DOUBLE_SIGN_BIT_SET(reg2_data);
            }
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_Z);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_Z);
        } else {
            result_data.data = (double)1/reg2_data.data;
        }
    }
    end_float_op(cpu, &ex);
	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: RECIPF.D r%d(%lf),r%d(%fl):%lf\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//	printf("0x%x: RECIPF.D r%d(%lf),r%d(%lf):%lf\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);

	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];
	cpu->reg.pc += 4;

    return 0;

}
int op_exec_rsqrtf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
    uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}

	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = 1/sqrt(reg2_data.data);
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);
	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: RSQRTF.D r%d(%lf),r%d(%fl):%lf\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//	printf("0x%x: RSQRF.D r%d(%lf),r%d(%lf):%lf\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);

	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];
	cpu->reg.pc += 4;

    return 0;

    
}
int op_exec_sqrtf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
    uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}

	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = sqrt(reg2_data.data);
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);
	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: SQRTF.D r%d(%lf),r%d(%fl):%lf\n", 
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//	printf("0x%x: SQRF.D r%d(%lf),r%d(%lf):%lf\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);

	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];
	cpu->reg.pc += 4;

    return 0;
}
int op_exec_subf_d_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg1_0 = cpu->decoded_code->type_f.reg1;
	uint32 reg1_1 = cpu->decoded_code->type_f.reg1 + 1;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg1_data;
    DoubleBinaryDataType reg2_data;
    DoubleBinaryDataType reg3_data;
    DoubleBinaryDataType result_data;

	if (reg1_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
	reg1_data.binary[0] = cpu->reg.r[reg1_0];
	reg1_data.binary[1] = cpu->reg.r[reg1_1];
	reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
	reg3_data.binary[0] = cpu->reg.r[reg3_0];
	reg3_data.binary[1] = cpu->reg.r[reg3_1];

    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg1_data);
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data.data = reg2_data.data - reg1_data.data;
        set_subnormal_result_double(cpu, &fpu_config, &result_data);
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: SUBF_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n",
        cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data));
//	printf("0x%x: SUBF_D r%d(%lf),r%d(%lf),r%d(%lf):%lf\n",cpu->reg.pc, reg1_0, reg1_data.data, reg2_0, reg2_data.data, reg3_0, reg3_data.data, result_data.data);

	cpu->reg.r[reg3_0] = result_data.binary[0];
	cpu->reg.r[reg3_1] = result_data.binary[1];

	cpu->reg.pc += 4;
    return 0;

}
int op_exec_trncf_dl_F(TargetCoreType *cpu)
{
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    sint64 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
    // TODO:Overflow
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        sint64 result;
        bool is_invalid = FALSE;
        if (DOUBLE_IS_NAN(reg2_data) || DOUBLE_IS_INF(reg2_data)) {
            if (DOUBLE_IS_PLUS(reg2_data)) {
                result = (sint64)LONG_MAX;
            }
            else {
                result = (sint64)( -((sint64)LONG_MAX) );
            }
            is_invalid = TRUE;
        }
        else {
            result = (sint64)trunc(reg2_data.data);
            if ( result < ((sint64)( -((sint64)LONG_MAX))) ) {
                result = (sint64)( -((sint64)LONG_MAX) );
                is_invalid = TRUE;
            } 
            else if (result > ((sint64)LONG_MAX)) {
                result = (sint64)LONG_MAX;
                is_invalid = TRUE;
            }
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data = (sint64)result;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF.DL r%d(%lf) : r%d :%lld\n",
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data));
//	printf( "0x%x: TRNCF.DL r%d(%lf) r%d :%lld\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data);
	cpu->reg.r[reg3_0] = (uint32)(result_data>>32);
	cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;

	return 0;

}
int op_exec_trncf_dul_F(TargetCoreType *cpu)
{
    // Double floating to unsingned long word
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    DoubleBinaryDataType reg2_data;
    uint64 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
 
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        uint64 result;
        bool is_invalid = TRUE;
        if (DOUBLE_IS_NAN(reg2_data)) {
            result = 0;
        } else if ( DOUBLE_IS_INF(reg2_data)) {
            if (DOUBLE_IS_PLUS(reg2_data)) {
                result = (uint64)ULONG_MAX;
            } else {
                result = 0;
            }
        } else if ( reg2_data.data < 0 ) {
            result = 0;
        } else if ( reg2_data.data > (uint64)ULONG_MAX) {
            result = (uint64)ULONG_MAX;
        } else {
            result = (uint64)trunc(reg2_data.data);
            is_invalid = FALSE;
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data = (uint64)result;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF.DUL r%d(%lf) : r%d :%llu\n",
        cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data));
//	printf( "0x%x: TRNCF.DUL r%d(%lf) r%d :%llu\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3_0,result_data);
	cpu->reg.r[reg3_0] = (uint32)(result_data>>32);
	cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;

	return 0;
}
int op_exec_trncf_duw_F(TargetCoreType *cpu)
{
    // Double floating to unsingned long word
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3   = cpu->decoded_code->type_f.reg3;
    DoubleBinaryDataType reg2_data;
    uint32 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
 
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (uint32)reg2_data.data;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF.DUW r%d(%lf) : r%d :%u\n",
        cpu->reg.pc, reg2_0, reg2_data.data, reg3,result_data));
//    printf("0x%x: TRNCF.DW r%d(%lf) : r%d :%u\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3,result_data);
	cpu->reg.r[reg3] = (uint32)(result_data);
	cpu->reg.pc += 4;

	return 0;

}
int op_exec_trncf_dw_F(TargetCoreType *cpu)
{
    // Double floating to unsingned long word
    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2_0 = cpu->decoded_code->type_f.reg2;
	uint32 reg2_1 = cpu->decoded_code->type_f.reg2 + 1;
	uint32 reg3   = cpu->decoded_code->type_f.reg3;
    DoubleBinaryDataType reg2_data;
    sint32 result_data;

	if (reg2_1 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary[0] = cpu->reg.r[reg2_0];
	reg2_data.binary[1] = cpu->reg.r[reg2_1];
 
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        set_subnormal_operand_double(cpu, &fpu_config, &reg2_data);
        result_data = (sint32)reg2_data.data;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF.DW r%d(%lf) : r%d :%d\n",
        cpu->reg.pc, reg2_0, reg2_data.data, reg3,result_data));
//    printf("0x%x: TRNCF.DW r%d(%lf) : r%d :%d\n", cpu->reg.pc, reg2_0, reg2_data.data, reg3,result_data);
	cpu->reg.r[reg3] = (uint32)(result_data);
	cpu->reg.pc += 4;

	return 0;
}

int op_exec_trncf_sl_F(TargetCoreType *cpu)
{
    // single floating to 64bit fixed-point

    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    FloatBinaryDataType reg2_data;
    sint64 result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary = cpu->reg.r[reg2];
    
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        bool is_invalid = TRUE;
        sint64 result;
        if (FLOAT_IS_NAN(reg2_data) ) {
            result = (sint64)-LONG_MAX;
        } else if (FLOAT_IS_INF(reg2_data) ) {
            result = (sint64)LONG_MAX;
        } else {
            result = (sint64)truncf(reg2_data.data);
            is_invalid = FALSE;
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data = (sint64)result;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF.SL r%d(%f) r%d :%lld\n",
        cpu->reg.pc, reg2, reg2_data.data, reg3_0,result_data));
//	printf( "0x%x: TRNCF.SL r%d(%f) r%d :%lld\n", cpu->reg.pc, reg2, reg2_data.data, reg3_0,result_data);
	cpu->reg.r[reg3_0] = (uint32)(result_data>>32);
	cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;

	return 0;

}

int op_exec_trncf_sul_F(TargetCoreType *cpu)
{
    // single floating to unsigned 64bit fixed

    FpuConfigSettingType fpu_config;
    FloatExceptionType ex;
	uint32 reg2 = cpu->decoded_code->type_f.reg2;
	uint32 reg3_0 = cpu->decoded_code->type_f.reg3;
	uint32 reg3_1 = cpu->decoded_code->type_f.reg3 + 1;
    FloatBinaryDataType reg2_data;
    uint64 result_data;

	if (reg2 >= CPU_GREG_NUM) {
		return -1;
	}
	if (reg3_1 >= CPU_GREG_NUM) {
		return -1;
	}
    reg2_data.binary = cpu->reg.r[reg2];
    
    prepare_float_op(cpu, &ex, &fpu_config);
    {
        bool is_invalid = TRUE;
        uint64 result;
        if (FLOAT_IS_NAN(reg2_data) ) {
            result = 0;
        } else if (FLOAT_IS_INF(reg2_data) ) {
            if ( FLOAT_IS_PLUS(reg2_data)) {
                result = (uint64)ULONG_MAX;
            } else {   
                result = 0;
            }
        } else {
            if ( reg2_data.data < 0 ) {
                result = 0;
            } else if ( reg2_data.data > (uint64)ULONG_MAX) {
                result = (uint64)ULONG_MAX;
            } else {
                result = (uint64)truncf(reg2_data.data);
                is_invalid = FALSE;
            }
        }
        if (is_invalid) {
            sys_set_fpst_xc(&cpu->reg, sys_get_fpst_xc(&cpu->reg) | SYS_FPSR_EXPR_V);
            sys_set_fpst_xp(&cpu->reg, sys_get_fpst_xp(&cpu->reg) | SYS_FPSR_EXPR_V);
        }
        result_data = (uint64)result;
    }
    end_float_op(cpu, &ex);

	DBG_PRINT((DBG_EXEC_OP_BUF(), DBG_EXEC_OP_BUF_LEN(), "0x%x: TRNCF.SUL r%d(%f) r%d :%llu\n",
        cpu->reg.pc, reg2, reg2_data.data, reg3_0,result_data));
//	printf( "0x%x: TRNCF.SUL r%d(%f) r%d :%llu\n", cpu->reg.pc, reg2, reg2_data.data, reg3_0,result_data);
	cpu->reg.r[reg3_0] = (uint32)(result_data>>32);
	cpu->reg.r[reg3_1] = (uint32)(result_data & 0xffffffff);
	cpu->reg.pc += 4;

	return 0;

}

int op_exec_ceilf_sl_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_ceilf_sul_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_ceilf_suw_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_ceilf_sw_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_cvtf_sl_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_cvtf_sul_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_floorf_sl_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_floorf_sul_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_floorf_suw_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_floorf_sw_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_rsqrtf_s_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
int op_exec_sqrtf_s_F(TargetCoreType *cpu)
{
	printf("ERROR: not supported:%s\n", __FUNCTION__);
	return -1;
}
