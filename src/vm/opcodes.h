/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_OPCODES_H__
#define __LR1_OPCODES_H__

#include <cstdint>
#include <string>


using t_vm_byte = std::uint8_t;
using t_vm_addr = std::int32_t;
using t_vm_real = double;
using t_vm_int = std::int64_t;
using t_vm_bool = t_vm_byte;
using t_vm_str = std::string;

using t_vm_longest_type = t_vm_real;


template<class t_val> std::string vm_type_name = "unknown";
template<> inline std::string vm_type_name<t_vm_byte> = "byte";
template<> inline std::string vm_type_name<t_vm_addr> = "address";
template<> inline std::string vm_type_name<t_vm_real> = "real";
template<> inline std::string vm_type_name<t_vm_int> = "integer";
template<> inline std::string vm_type_name<t_vm_str> = "string";


enum class VMType : t_vm_byte
{
	UNKNOWN  = 0x00,
	REAL     = 0x01,
	INT      = 0x02,
	BOOLEAN  = 0x03,
	STR      = 0x04,

	ADDR_MEM = 0b00001000,  // address refering to absolute memory locations
	ADDR_IP  = 0b00001001,  // address relative to the instruction pointer
	ADDR_SP  = 0b00001010,  // address relative to the stack pointer
	ADDR_BP  = 0b00001011,  // address relative to a local base pointer
	ADDR_GBP = 0b00001100,  // address relative to the global base pointer
};


// VMType sizes (including data type and, optionally, descriptor byte)
template<VMType ty, bool with_descr = false> constexpr t_vm_addr vm_type_size
	= sizeof(t_vm_longest_type) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::UNKNOWN, with_descr>
	= sizeof(t_vm_longest_type) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::REAL, with_descr>
	= sizeof(t_vm_real) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::INT, with_descr>
	= sizeof(t_vm_int) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::BOOLEAN, with_descr>
	= sizeof(t_vm_bool) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_MEM, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_IP, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_SP, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_BP, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_GBP, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);


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
