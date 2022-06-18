/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_OPCODES_H__
#define __LR1_OPCODES_H__

#include <cstdint>


using t_vm_addr = std::int16_t;
using t_vm_byte = std::int8_t;


enum class VMType : t_vm_byte
{
	UNKNOWN  = 0x00,
	REAL     = 0x01,
	INT      = 0x02,
	BOOLEAN  = 0x03,

	ADDR_MEM = 0b00001000,  // address refering to absolute memory locations
	ADDR_IP  = 0b00001001,  // address relative to the instruction pointer
	ADDR_SP  = 0b00001010,  // address relative to the stack pointer
	ADDR_BP  = 0b00001011,  // address relative to a local base pointer
	ADDR_GBP = 0b00001100,  // address relative to the global base pointer
};


enum class OpCode : t_vm_byte
{
	HALT     = 0x00,  // stop program
	NOP      = 0x01,  // no operation
	INVALID  = 0x02,  // invalid opcode

	// memory operations
	PUSH     = 0x10,  // push direct data
	DEREF    = 0x11,  // dereference pointer
	WRMEM    = 0x12,  // write mem
	RDMEM    = 0x13,  // read mem

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
	CALL     = 0x70,
	RET      = 0x71,
};


#endif
