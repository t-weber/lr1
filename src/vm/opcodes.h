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
	BOOLEAN  = 0x03,
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
	HALT     = 0x00,  // stop program
	NOP      = 0x01,  // no operation
	INVALID  = 0x02,  // invalid opcode

	// memory operations
	PUSH     = 0x10,  // push direct data
	PUSHADDR = 0x11,  // get the address of a variable
	DEREF    = 0x12,  // dereference pointer
	WRMEM    = 0x13,  // write mem
	RDMEM    = 0x14,  // read mem

	// arithmetic operations
	USUB     = 0x20,  // unary -
	ADD      = 0x21,  // +
	SUB      = 0x22,  // -
	MUL      = 0x23,  // *
	DIV      = 0x24,  // /
	MOD      = 0x25,  // %
	POW      = 0x26,  // ^

	// conversions
	TOI      = 0x30,  // cast to int
	TOF      = 0x31,  // cast to real

	// jumps
	JMP      = 0x40,  // unconditional jump to direct address
	JMPCND   = 0x41,  // conditional jump to direct address
	SKIP     = 0x42,  // unconditional relative jump
	SKIPCND  = 0x43,  // conditional relative jump

	// logical operations
	AND      = 0x50,  // &&
	OR       = 0x51,  // ||
	XOR      = 0x52,  // ^
	NOT      = 0x53,  // !

	// comparisons
	GT       = 0x60,  // >
	LT       = 0x61,  // <
	GEQU     = 0x62,  // >=
	LEQU     = 0x63,  // <=
	EQU      = 0x64,  // ==
	NEQU     = 0x65,  // !=

	// function calls
	// TODO
};


#endif
