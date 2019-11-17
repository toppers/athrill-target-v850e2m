#ifndef _TIMER_H_
#define _TIMER_H_

#include "device.h"

/*************************************************
 * 16ビットタイマ／イベントカウンタAA(TAA)
 *************************************************/

#define TAAnChannelNum			UINT_C(8)
#define TAAnCH0					UINT_C(0)
#define TAAnCH1					UINT_C(1)
#define TAAnCH2					UINT_C(2)
#define TAAnCH3					UINT_C(3)
#define TAAnCH4					UINT_C(4)
#define TAAnCH5					UINT_C(5)
#define TAAnCH6					UINT_C(6)
#define TAAnCH7					UINT_C(7)

/*
 * TAAn制御レジスタ0
 */
#define TAAnCTL0_BASE			UINT_C(0xFFFFF590)
#define TAAnCTL0(CH)			(TAAnCTL0_BASE + ((CH) * 4U))
/*
 * TAAn制御レジスタ1
 */
#define TAAnCTL1_BASE			UINT_C(0xFFFFF5A0)
#define TAAnCTL1(CH)			(TAAnCTL1_BASE + ((CH) * 4U))


/*
 * TAAn キャプチャ／コンペア・レジスタ 0（ TAAnCCR0）
 */
#define TAAnCCR0_BASE			UINT_C(0xFFFFF5C0)
#define TAAnCCR0(CH)			(TAAnCCR0_BASE + ((CH) * 4U))

/*
 * TAAn キャプチャ／コンペア・レジスタ 1（ TAAnCCR1）
 */
#define TAAnCCR1_BASE			UINT_C(0xFFFFF5E0)
#define TAAnCCR1(CH)			(TAAnCCR1_BASE + ((CH) * 4U))

/*
 * TAAnカウンタ・リード・バッファ・レジスタ
 */
#define TAAnCNT_BASE			UINT_C(0xFFFFF610)
#define TAAnCNT(CH)				(TAAnCNT_BASE + ((CH) * 4U))

/*
 * TAAn オプション・レジスタ 0（ TAAnOPT0）
 */
#define TAAnOPT0_BASE			UINT_C(0xFFFFF630)
#define TAAnOPT0(CH)			(TAAnOPT0_BASE + ((CH) * 4U))



#endif /* _TIMER_H_ */
