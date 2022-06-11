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


enum class VMType : t_vm_byte
{
	REAL     = 0x01,
	INT      = 0x02,
};


enum class VMRegister : t_vm_byte
{
	MEM      = 0x00,

	IP       = 0x01,	// instruction pointer
	SP       = 0x02,	// stack pointer
	BP       = 0x03,	// frame base pointer for local variables
	GBP      = 0x04,	// global base pointer
};


enum class OpCode : t_vm_byte
{
	HALT     = 0x00,
	NOP      = 0x01,
	INVALID  = 0x02,

	// memory operations
	PUSH     = 0x10,
	PUSHADDR = 0x11,
	DEREF    = 0x12,
	WRMEM    = 0x13,
	RDMEM    = 0x14,

	// arithmetics
	USUB     = 0x20,
	ADD      = 0x21,
	SUB      = 0x22,
	MUL      = 0x23,
	DIV      = 0x24,
	MOD      = 0x25,
	POW      = 0x26,

	// conversions
	FTOI     = 0x30,
	ITOF     = 0x31,
};


#endif
