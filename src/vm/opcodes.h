/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_OPCODES_H__
#define __LR1_OPCODES_H__

#include <cstdint>


using t_vm_addr = std::uint16_t;
using t_vm_byte = std::int8_t;


enum class Register : t_vm_byte
{
	MEM = 0x00,

	IP = 0x01,		// instruction pointer
	SP = 0x02,		// stack pointer
	BP = 0x03,		// frame base pointer for local variables
	GBP = 0x04,		// global base pointer
};


enum class OpCode : t_vm_byte
{
	HALT = 0x00,
	NOP = 0x01,
	INVALID = 0x02,

	// real memory operations
	PUSHADDR = 0x03,
	PUSHF = 0x04,
	DEREFF = 0x05,
	WRMEMF = 0x06,
	RDMEMF = 0x07,

	// int memory operations
	PUSHI = 0x08,
	DEREFI = 0x09,
	WRMEMI = 0x0a,
	RDMEMI = 0x0b,

	// real arithmetics
	USUBF = 0x10,
	ADDF = 0x11,
	SUBF = 0x12,
	MULF = 0x13,
	DIVF = 0x14,
	MODF = 0x15,
	POWF = 0x16,
	FTOI = 0x17,

	// int arithmetics
	USUBI = 0x20,
	ADDI = 0x21,
	SUBI = 0x22,
	MULI = 0x23,
	DIVI = 0x24,
	MODI = 0x25,
	POWI = 0x26,
	ITOF = 0x27,
};


#endif
