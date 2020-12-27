#ifndef _CPU_FPU_OPS_H_
#define _CPU_FPU_OPS_H_

#include "target_cpu.h"

#ifndef DEFINE_FLOAT_TYPEDEF
typedef float float32;
typedef double float64;
#endif

#define FLOAT_SIGN_BIT_POS		31
#define FLOAT_SIGN_BIT_PTRN		(1U << FLOAT_SIGN_BIT_POS)
#define FLOAT_SIGN_BIT_MASK		~(1U << FLOAT_SIGN_BIT_POS)
#define GET_FLOAT_SIGN_BIT_VALUE(x) ( (((FLOAT_SIGN_BIT_PTRN) & (x).binary) != 0) ? 1U : 0U )
#define FLOAT_SIGN_BIT_SET(x)	\
do {	\
	(x).binary |= FLOAT_SIGN_BIT_PTRN;	\
} while (0)

#define FLOAT_SIGN_BIT_CLR(x)	\
do {	\
	(x).binary &= FLOAT_SIGN_BIT_MASK;	\
} while (0)

#define DOUBLE_SIGN_BIT_POS		FLOAT_SIGN_BIT_POS
#define DOUBLE_SIGN_BIT_PTRN	FLOAT_SIGN_BIT_PTRN
#define DOUBLE_SIGN_BIT_MASK		FLOAT_SIGN_BIT_MASK
#define GET_DOUBLE_SIGN_BIT_VALUE(x) ( (((DOUBLE_SIGN_BIT_PTRN) & (x).binary[1]) != 0) ? 1U : 0U )
#define DOUBLE_SIGN_BIT_SET(x)	\
do {	\
	(x).binary[1] |= DOUBLE_SIGN_BIT_PTRN;	\
} while (0)

#define DOUBLE_SIGN_BIT_CLR(x)	\
do {	\
	(x).binary[1] &= DOUBLE_SIGN_BIT_MASK;	\
} while (0)


#define FLOAT_EXP_BIT_POS_START		30
#define FLOAT_EXP_BIT_POS_END		23
#define FLOAT_EXP_BIT_MASK			0x7F800000
#define FLOAT_EXP_BIAS_VALUE		((sint16)127)
#define GET_FLOAT_EXP_VALUE(x) 		( (sint16)( ((FLOAT_EXP_BIT_MASK & (x).binary) >> FLOAT_EXP_BIT_POS_END) ) )
#define GET_FLOAT_EXPB_VALUE(x) 	( GET_FLOAT_EXP_VALUE(x) - FLOAT_EXP_BIAS_VALUE )


#define DOUBLE_EXP_BIT_POS_START	30
#define DOUBLE_EXP_BIT_POS_END		20
#define DOUBLE_EXP_BIT_MASK			0x7FE00000
#define DOUBLE_EXP_BIAS_VALUE		((sint16)1023)
#define GET_DOUBLE_EXP_VALUE(x) 		( (sint16)( ((DOUBLE_EXP_BIT_MASK & (x).binary[1]) >> DOUBLE_EXP_BIT_POS_END) ) )
#define GET_DOUBLE_EXPB_VALUE(x) 	( GET_DOUBLE_EXP_VALUE(x) - DOUBLE_EXP_BIAS_VALUE )


#define FLOAT_EXP_MAX				((sint16)+127)
#define FLOAT_EXP_MIN				((sint16)-126)
#define FLOAT_EXP_MAX_PLUS_1		((sint16)+128)
#define FLOAT_EXP_MIN_MINUS_1		((sint16)-127)


#define DOUBLE_EXP_MAX				((sint16)+1023)
#define DOUBLE_EXP_MIN				((sint16)-1022)
#define DOUBLE_EXP_MAX_PLUS_1		((sint16)+1024)
#define DOUBLE_EXP_MIN_MINUS_1		((sint16)-1023)


#define FLOAT_MANTISSA_POS_START	22
#define FLOAT_MANTISSA_POS_END		0
#define FLOAT_MANTISSA_BIT_MASK		0x007FFFFF
#define GET_FLOAT_MANTISSA_VALUE(x) ( ((FLOAT_MANTISSA_BIT_MASK & (x).binary) >> FLOAT_MANTISSA_POS_END))
#define GET_FLOAT_MANTISSA_BIT_VALUE(ma, bit_pos) ( ( ((ma) & (1U << (bit_pos)) ) != 0U) ? 1U : 0U )

#define DOUBLE_MANTISSA1_POS_START		19
#define DOUBLE_MANTISSA1_POS_END		0
#define DOUBLE_MANTISSA1_BIT_MASK		0x007FFFFF

#define DOUBLE_MANTISSA0_POS_START		31
#define DOUBLE_MANTISSA0_POS_END		0
#define DOUBLE_MANTISSA0_BIT_MASK		0xFFFFFFFF

#define GET_DOUBLE_MANTISSA1_VALUE(x) ( ((DOUBLE_MANTISSA1_BIT_MASK & (x).binary[1]) >> DOUBLE_MANTISSA1_POS_END))
#define GET_DOUBLE_MANTISSA0_VALUE(x) ( ((DOUBLE_MANTISSA0_BIT_MASK & (x).binary[0]) >> DOUBLE_MANTISSA0_POS_END))
#define GET_DOUBLE_MANTISSA_BIT_VALUE(ma, bit_pos) ( ( ((ma) & (1U << (bit_pos)) ) != 0U) ? 1U : 0U )


/*
 * 種類　　　　指数部　　　仮数部
 * ------------------------------
 * ゼロ　　　　０　　　　　　 ０
 * 非正規化数　０　　　　　　 ０以外
 * 正規化数　　1～254　　　	  任意
 * 無限大　　　255　　　　　  ０
 * NaN　　　　 255　　　　　  ０以外
 */
#define FLOAT_IS_ZERO(x)	( (GET_FLOAT_EXP_VALUE(x) == 0)   && (GET_FLOAT_MANTISSA_VALUE(x) == 0  ) )
#define FLOAT_IS_NORMAL(x)	( (GET_FLOAT_EXP_VALUE(x) >= 1)   && (GET_FLOAT_EXP_VALUE(x)      <= 254) )
#define FLOAT_IS_SBNORM(x)	( (GET_FLOAT_EXP_VALUE(x) == 0)   && (GET_FLOAT_MANTISSA_VALUE(x) != 0  ) )
#define FLOAT_IS_INF(x)		( (GET_FLOAT_EXP_VALUE(x) == 255) && (GET_FLOAT_MANTISSA_VALUE(x) == 0  ) )
#define FLOAT_IS_NAN(x)		( (GET_FLOAT_EXP_VALUE(x) == 255) && (GET_FLOAT_MANTISSA_VALUE(x) != 0  ) )
#define FLOAT_IS_PLUS(x)	( GET_FLOAT_SIGN_BIT_VALUE(x) == 0 )
#define FLOAT_IS_MINUS(x)	( GET_FLOAT_SIGN_BIT_VALUE(x) == 1 )


#define DOUBLE_IS_ZERO(x)	( (GET_DOUBLE_EXP_VALUE(x) == 0)   && (GET_DOUBLE_MANTISSA0_VALUE(x) == 0  ) && (GET_DOUBLE_MANTISSA1_VALUE(x) == 0  ) )
#define DOUBLE_IS_NORMAL(x)	( (GET_DOUBLE_EXP_VALUE(x) >= 1)   && (GET_DOUBLE_EXP_VALUE(x)      <= 2046) )
#define DOUBLE_IS_SBNORM(x)	( (GET_DOUBLE_EXP_VALUE(x) == 0)   && ( (GET_DOUBLE_MANTISSA0_VALUE(x) != 0  ) || (GET_DOUBLE_MANTISSA1_VALUE(x) != 0  )) )
#define DOUBLE_IS_INF(x)	( (GET_DOUBLE_EXP_VALUE(x) == 2047) && (GET_DOUBLE_MANTISSA0_VALUE(x) == 0  ) && (GET_DOUBLE_MANTISSA1_VALUE(x) == 0  ) )
#define DOUBLE_IS_NAN(x)	( (GET_DOUBLE_EXP_VALUE(x) == 2047) && ( (GET_DOUBLE_MANTISSA0_VALUE(x) != 0  ) || (GET_DOUBLE_MANTISSA1_VALUE(x) != 0  )) )
#define DOUBLE_IS_PLUS(x)	( GET_DOUBLE_SIGN_BIT_VALUE(x) == 0 )
#define DOUBLE_IS_MINUS(x)	( GET_DOUBLE_SIGN_BIT_VALUE(x) == 1 )


#define FLOAT_SET_SIGN_REVERSE(x)	\
do { \
	if (FLOAT_IS_PLUS(x)) {	\
		FLOAT_SIGN_BIT_SET(x);	\
	}	\
	else {	\
		FLOAT_SIGN_BIT_CLR(x);	\
	}	\
} while (0)

#define DOUBLE_SET_SIGN_REVERSE(x)	\
do { \
	if (DOUBLE_IS_PLUS(x)) {	\
		DOUBLE_SIGN_BIT_SET(x);	\
	}	\
	else {	\
		DOUBLE_SIGN_BIT_CLR(x);	\
	}	\
} while (0)


typedef union {
	float32		data;
	uint32 		binary;
} FloatBinaryDataType;

typedef union {
	float64		data;
	uint32 		binary[2];
} DoubleBinaryDataType;

static inline bool float_is_qnan(FloatBinaryDataType x)
{
	if (!FLOAT_IS_NAN(x)) {
		return FALSE;
	}
	if ((x.binary & (1U << FLOAT_MANTISSA_POS_START)) != 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}
static inline bool float_is_snan(FloatBinaryDataType x)
{
	if (!FLOAT_IS_NAN(x)) {
		return FALSE;
	}
	if ((x.binary & (1U << FLOAT_MANTISSA_POS_START)) != 0) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

static inline void float_get_qnan(FloatBinaryDataType *x)
{
	x->binary = 0x7FFFFFFF;
	return;
}
static inline void float_get_snan(FloatBinaryDataType *x)
{
	x->binary = ( 0x7FFFFFFF & ~(1U << FLOAT_MANTISSA_POS_START) );
	return;
}


static inline bool double_is_qnan(DoubleBinaryDataType x)
{
	if (!DOUBLE_IS_NAN(x)) {
		return FALSE;
	}
	if ((x.binary[1] & (1U << DOUBLE_MANTISSA1_POS_START)) != 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}
static inline bool double_is_snan(DoubleBinaryDataType x)
{
	if (!DOUBLE_IS_NAN(x)) {
		return FALSE;
	}
	if ((x.binary[1] & (1U << DOUBLE_MANTISSA1_POS_START)) != 0) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

static inline void double_get_qnan(DoubleBinaryDataType *x)
{
	x->binary[1] = 0x7FFFFFFF;
	x->binary[0] = 0xFFFFFFFF;
	return;
}
static inline void double_get_snan(DoubleBinaryDataType *x)
{
	x->binary[1] = ( 0x7FFFFFFF & ~(1U << DOUBLE_MANTISSA1_POS_START) );
	x->binary[0] = 0xFFFFFFFF;
	return;
}

/*
 * (1) 不正確演算例外（ I）
 * (2) 無効演算例外（ V）
 * (3) ゼロ除算例外（ Z）
 * (4) オーバフロー例外（ O）
 * (5) アンダフロー例外（ U）
 * (6) 未実装演算例外（ E）
 */
 #define FLOAT_EXCEPTION_BIT_I		0U
 #define FLOAT_EXCEPTION_BIT_V		1U
 #define FLOAT_EXCEPTION_BIT_Z		2U
 #define FLOAT_EXCEPTION_BIT_O		3U
 #define FLOAT_EXCEPTION_BIT_U		4U
 #define FLOAT_EXCEPTION_BIT_E		5U
typedef struct {
	uint32 disable_bits;
	uint32 intr_bits;
} FloatExceptionType;

#define FLOAT_EXCEPTION_IS_RAISED(fex, bitpos)	( ( (fex).intr_bits & (1U << (bitops)) ) != 0 )
#define FLOAT_EXCEPTION_IS_DISABLE(fex, bitpos)	( ( (fex).disable_bits & (1U << (bitops)) ) != 0 )

#define FLOAT_EXCEPTION_BIT_RESET(fex)	\
do {	\
	(fex)->disable_bits = 0U;	\
	(fex)->intr_bits = 0U;	\
} while (0)

#define FLOAT_EXCEPTION_INTR_BIT_SET(fex, bitpos)	\
do { \
	(fex)->intr_bits |= (1U << (bitpos));	\
} while (0)

#define FLOAT_EXCEPTION_INTR_BIT_CLR(fex, bitpos)	\
do { \
	(fex)->intr_bits &= ~(1U << (bitpos));	\
} while (0)

#define FLOAT_EXCEPTION_DISABLE_BIT_SET(fex, bitpos)	\
do { \
	(fex)->disable_bits |= (1U << (bitpos));	\
} while (0)

#define FLOAT_EXCEPTION_DISABLE_BIT_CLR(fex, bitpos)	\
do { \
	(fex)->disable_bits &= ~(1U << (bitpos));	\
} while (0)


static inline void op_absf_s(FloatBinaryDataType *in, FloatBinaryDataType *out)
{
	out->binary = (in->binary & FLOAT_SIGN_BIT_MASK);
	return;
}


/**********************************************
 * FPSR
 **********************************************/
#define SYS_FPSR_CC_BIT_START	24
#define SYS_FPSR_FN_BIT_START	23
#define SYS_FPSR_IF_BIT_START	22
#define SYS_FPSR_PEM_BIT_START	21
#define SYS_FPSR_RM_BIT_START	18
#define SYS_FPSR_FS_BIT_START	17
#define SYS_FPSR_XC_BIT_START	10
#define SYS_FPSR_XE_BIT_START	 5
#define SYS_FPSR_XP_BIT_START	 0

#define SYS_FPST_XC_BIT_START	8
#define SYS_FPST_IF_BIT_START	5
#define SYS_FPST_XP_BIT_START	0

#define SYS_FPCC_CC_BIT_START	0

#define SYS_FPCFG_RM_BIT_START	8
#define SYS_FPCFG_XE_BIT_START	0

static inline void sys_set_fpsr_cc(CpuRegisterType *reg, uint8 data)
{
	/*
	 * FPSR
	 */
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_DATA8(regp, SYS_FPSR_CC_BIT_START, data);

	/*
	 * FPCC
	 */
	regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPCC);
	SYS_SET_DATA8(regp, SYS_FPCC_CC_BIT_START, data);
	return;
}
static inline uint8 sys_get_fpsr_cc(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return SYS_GET_DATA8(regp, SYS_FPSR_CC_BIT_START);
}
static inline bool sys_isset_fpsr_fn(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return (SYS_ISSET_BIT(regp, SYS_FPSR_FN_BIT_START) != 0);
}

static inline void sys_set_fpsr_fn(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_BIT(regp, SYS_FPSR_FN_BIT_START);
	return;
}
static inline void sys_clr_fpsr_fn(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_CLR_BIT(regp, SYS_FPSR_FN_BIT_START);
	return;
}

static inline bool sys_isset_fpsr_if(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return (SYS_ISSET_BIT(regp, SYS_FPSR_IF_BIT_START) != 0);
}
static inline void sys_set_fpsr_if(CpuRegisterType *reg)
{
	/*
	 * FPSR
	 */
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_BIT(regp, SYS_FPSR_IF_BIT_START);

	/*
	 * FPST
	 */
	regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	SYS_SET_BIT(regp, SYS_FPST_IF_BIT_START);
	return;
}
static inline void sys_clr_fpsr_if(CpuRegisterType *reg)
{
	/*
	 * FPSR
	 */
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_CLR_BIT(regp, SYS_FPSR_IF_BIT_START);

	/*
	 * FPST
	 */
	regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	SYS_CLR_BIT(regp, SYS_FPST_IF_BIT_START);
	return;
}

static inline bool sys_isset_fpsr_pem(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return (SYS_ISSET_BIT(regp, SYS_FPSR_PEM_BIT_START) != 0);
}
static inline void sys_set_fpsr_pem(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_BIT(regp, SYS_FPSR_PEM_BIT_START);
	return;
}
static inline void sys_clr_fpsr_pem(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_CLR_BIT(regp, SYS_FPSR_PEM_BIT_START);
	return;
}

typedef enum {
	SysFpuRm_RN = 0x00,
	SysFpuRm_RZ = 0x01,
	SysFpuRm_RP = 0x10,
	SysFpuRm_RD = 0x11,
} SysFpuRmType;
static inline SysFpuRmType sys_get_fpsr_rm(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return SYS_GET_DATA2(regp, SYS_FPSR_RM_BIT_START);
}

static inline void sys_set_fpsr_rm(CpuRegisterType *reg, SysFpuRmType data)
{
	/*
	 * FPSR
	 */
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_DATA2(regp, SYS_FPSR_RM_BIT_START, (uint8)data);

	/*
	 * FPCFG
	 */
	regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPCFG);
	SYS_SET_DATA2(regp, SYS_FPCFG_RM_BIT_START, (uint8)data);
	return;
}

static inline bool sys_isset_fpsr_fs(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return (SYS_ISSET_BIT(regp, SYS_FPSR_FS_BIT_START) != 0);
}
static inline void sys_set_fpsr_fs(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_BIT(regp, SYS_FPSR_FS_BIT_START);
	return;
}
static inline void sys_clr_fpsr_fs(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_CLR_BIT(regp, SYS_FPSR_FS_BIT_START);
	return;
}

#define SYS_FPSR_EXPR_I		(1U << 0U)
#define SYS_FPSR_EXPR_U		(1U << 1U)
#define SYS_FPSR_EXPR_O		(1U << 2U)
#define SYS_FPSR_EXPR_Z		(1U << 3U)
#define SYS_FPSR_EXPR_V		(1U << 4U)
#define SYS_FPSR_EXPR_E		(1U << 5U)
static inline void sys_set_fpsr_xc(CpuRegisterType *reg, uint8 data)
{
	/*
	 * FPSR
	 */
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_DATA6(regp, SYS_FPSR_XC_BIT_START, (uint8)data);

	/*
	 * FPST
	 */
	regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	SYS_SET_DATA6(regp, SYS_FPST_XC_BIT_START, (uint8)data);
	return;
}
static inline bool sys_isset_fpsr_xc(CpuRegisterType *reg, uint8 data)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return ((SYS_GET_DATA6(regp, SYS_FPSR_XC_BIT_START) & data) != 0);
}
static inline void sys_set_fpsr_xe(CpuRegisterType *reg, uint8 data)
{
	/*
	 * FPSR
	 */
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_DATA5(regp, SYS_FPSR_XE_BIT_START, (uint8)data);

	/*
	 * FPCFG
	 */
	regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPCFG);
	SYS_SET_DATA5(regp, SYS_FPCFG_XE_BIT_START, (uint8)data);
	return;
}
static inline bool sys_isset_fpsr_xe(CpuRegisterType *reg, uint8 data)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return ((SYS_GET_DATA5(regp, SYS_FPSR_XE_BIT_START) & data) != 0);
}
static inline void sys_set_fpsr_xp(CpuRegisterType *reg, uint8 data)
{
	/*
	 * FPSR
	 */
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	SYS_SET_DATA5(regp, SYS_FPSR_XP_BIT_START, (uint8)data);

	/*
	 * FPST
	 */
	regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	SYS_SET_DATA5(regp, SYS_FPST_XP_BIT_START, (uint8)data);

	return;
}
static inline bool sys_isset_fpsr_xp(CpuRegisterType *reg, uint8 data)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPSR);
	return ((SYS_GET_DATA5(regp, SYS_FPSR_XP_BIT_START) & data) != 0);
}

/**********************************************
 * FPEPC
 **********************************************/
static inline uint32 sys_get_fpepc_pc(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPEC);
	return *regp;
}
static inline void sys_set_fpepc_pc(CpuRegisterType *reg, uint32 data)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPEC);
	*regp = ((data) & 0xFFFFFFFE);
	return;
}

/**********************************************
 * FPST
 **********************************************/
static inline void sys_set_fpst_xc(CpuRegisterType *reg, uint8 data)
{
	sys_set_fpsr_xc(reg, data);
	return;
}
static inline uint8 sys_get_fpst_xc(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	return SYS_GET_DATA6(regp, SYS_FPST_XC_BIT_START);
}
static inline bool sys_isset_fpst_xc(CpuRegisterType *reg, uint8 data)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	return ((SYS_GET_DATA6(regp, SYS_FPST_XC_BIT_START) & data) != 0);
}

static inline void sys_set_fpst_xp(CpuRegisterType *reg, uint8 data)
{
	sys_set_fpsr_xp(reg, data);
	return;
}
static inline uint8 sys_get_fpst_xp(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	return SYS_GET_DATA5(regp, SYS_FPST_XP_BIT_START);
}

static inline bool sys_isset_fpst_xp(CpuRegisterType *reg, uint8 data)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	return ((SYS_GET_DATA5(regp, SYS_FPST_XP_BIT_START) & data) != 0);
}
static inline bool sys_isset_fpst_if(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPST);
	return (SYS_ISSET_BIT(regp, SYS_FPST_IF_BIT_START) != 0);
}
static inline void sys_set_fpst_if(CpuRegisterType *reg)
{
	sys_set_fpsr_if(reg);
	return;
}
static inline void sys_clr_fpst_if(CpuRegisterType *reg)
{
	sys_clr_fpsr_if(reg);
	return;
}

/**********************************************
 * FPCC
 **********************************************/

static inline void sys_set_fpcc_cc(CpuRegisterType *reg, uint8 data)
{
	sys_set_fpsr_cc(reg, data);
	return;
}
static inline uint8 sys_get_fpcc_cc(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPCC);
	return SYS_GET_DATA8(regp, SYS_FPCC_CC_BIT_START);
}

/**********************************************
 * FPCFG
 **********************************************/

static inline SysFpuRmType sys_get_fpcfg_rm(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPCFG);
	return SYS_GET_DATA2(regp, SYS_FPCFG_RM_BIT_START);
}
static inline void sys_set_fpcfg_rm(CpuRegisterType *reg, SysFpuRmType data)
{
	sys_set_fpsr_rm(reg, data);
	return;
}

static inline void sys_set_fpcfg_xe(CpuRegisterType *reg, uint8 data)
{
	sys_set_fpsr_xe(reg, data);
	return;
}
static inline uint8 sys_get_fpcfg_xe(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPCFG);
	return SYS_GET_DATA5(regp, SYS_FPCFG_XE_BIT_START);
}

static inline bool sys_isset_fpcfg_xe(CpuRegisterType *reg, uint8 data)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPCFG);
	return ((SYS_GET_DATA5(regp, SYS_FPCFG_XE_BIT_START) & data) != 0);
}


/**********************************************
 * FPEC
 **********************************************/
#define SYS_FPEC_XE_BIT_START	0

static inline void sys_set_fpec_fpivd(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPEC);
	SYS_SET_BIT(regp, SYS_FPEC_XE_BIT_START);
	return;
}
static inline void sys_clr_fpec_fpivd(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPEC);
	SYS_CLR_BIT(regp, SYS_FPEC_XE_BIT_START);
	return;
}

static inline bool sys_isset_fpec_fpivd(CpuRegisterType *reg)
{
	uint32 *regp = cpu_get_sysreg_fpu(&reg->sys, SYS_REG_FPEC);
	return (SYS_ISSET_BIT(regp, SYS_FPEC_XE_BIT_START) != 0);
}

#endif /* _CPU_FPU_OPS_H_ */
