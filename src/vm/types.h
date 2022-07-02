/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_VM_TYPES_H__
#define __LR1_VM_TYPES_H__

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


/**
 * get derived data type for casting
 */
static inline VMType derive_data_type(VMType ty1, VMType ty2)
{
	if(ty1 == ty2)
		return ty1;
	else if(ty1 == VMType::STR || ty2 == VMType::STR)
		return VMType::STR;
	else if(ty1 == VMType::INT && ty2 == VMType::REAL)
		return VMType::REAL;
	else if(ty1 == VMType::REAL && ty2 == VMType::INT)
		return VMType::REAL;

	return VMType::UNKNOWN;
}


#endif
