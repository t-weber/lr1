/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_OPCODES_H__
#define __LR1_OPCODES_H__

#include "types.h"


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
	TOS      = 0x32,  // cast to string

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
	CALL     = 0x70,  // call function
	RET      = 0x71,  // return from function
	EXTCALL  = 0x72,  // call system function

	// binary operations
	BINAND   = 0x80,  // &
	BINOR    = 0x81,  // |
	BINXOR   = 0x82,  // ^
	BINNOT   = 0x83,  // ~
	SHL      = 0x84,  // <<
	SHR      = 0x85,  // >>
	ROTL     = 0x86,  // rotate left
	ROTR     = 0x87,  // rotate right
};


#endif
