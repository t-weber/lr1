/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_OPCODES_H__
#define __LR1_OPCODES_H__


#include <cstdint>


enum class OpCode : std::int8_t
{
	HALT = 0x00,

	PUSHF = 0x01,
	PUSHI = 0x02,

	// real arithmetics
	UADDF = 0x10,
	USUBF = 0x11,
	ADDF = 0x12,
	SUBF = 0x13,
	MULF = 0x14,
	DIVF = 0x15,
	MODF = 0x16,
	POWF = 0x17,
	FTOI = 0x18,

	// int arithmetics
	UADDI = 0x20,
	USUBI = 0x21,
	ADDI = 0x22,
	SUBI = 0x23,
	MULI = 0x24,
	DIVI = 0x25,
	MODI = 0x26,
	POWI = 0x27,
	ITOF = 0x28,
};


#endif
