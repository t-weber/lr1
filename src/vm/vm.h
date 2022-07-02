/**
 * zero-address code vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 8-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_0ACVM_H__
#define __LR1_0ACVM_H__


#include <type_traits>
#include <memory>
#include <variant>
#include <iostream>
#include <sstream>
#include <bit>
#include <string>
#include <cstring>
#include <cmath>

//#include "../codegen/lval.h"
#include "opcodes.h"
#include "helpers.h"


class VM
{
public:
	// data types
	using t_byte = ::t_vm_byte;
	using t_addr = ::t_vm_addr;
	using t_real = ::t_vm_real;
	using t_int = ::t_vm_int;
	using t_uint = typename std::make_unsigned<t_int>::type;
	using t_bool = ::t_vm_byte;
	using t_str = ::t_vm_str;
	using t_char = typename t_str::value_type;

	// variant of all data types
	using t_data = std::variant<t_real, t_int, t_bool, t_addr, t_str>;

	// data type sizes
	static constexpr const t_addr m_bytesize = sizeof(t_byte);
	static constexpr const t_addr m_addrsize = sizeof(t_addr);
	static constexpr const t_addr m_realsize = sizeof(t_real);
	static constexpr const t_addr m_intsize = sizeof(t_int);
	static constexpr const t_addr m_boolsize = sizeof(t_bool);

	// memory sizes and ranges
	t_addr m_memsize = 0x1000;
	t_addr m_framesize = 0x100;


public:
	VM(t_addr memsize = 1024);
	~VM() = default;

	void SetDebug(bool b) { m_debug = b; }

	void Reset();
	bool Run();

	void SetMem(t_addr addr, t_byte data);
	void SetMem(t_addr addr, const t_byte* data, std::size_t size);
	void SetMem(t_addr addr, const std::string& data);

	t_addr GetSP() const { return m_sp; }
	t_addr GetBP() const { return m_bp; }
	t_addr GetGBP() const { return m_gbp; }
	t_addr GetIP() const { return m_ip; }

	void SetSP(t_addr sp) { m_sp = sp; }
	void SetBP(t_addr bp) { m_bp = bp; }
	void SetGBP(t_addr bp) { m_gbp = bp; }
	void SetIP(t_addr ip) { m_ip = ip; }

	/**
	 * get top data from the stack
	 */
	t_data TopData() const;

	/**
	 * pop data from the stack
	 */
	t_data PopData();


protected:
	t_addr GetDataSize(const t_data& data) const;


	/**
	 * pop an address from the stack
	 */
	t_addr PopAddress();


	/**
	 * push an address to stack
	 */
	void PushAddress(t_addr addr, VMType ty = VMType::ADDR_MEM);

	/**
	 * pop a string from the stack
	 */
	t_str PopString();

	/**
	 * get the string on top of the stack
	 */
	t_str TopString(t_addr sp_offs = 0) const;

	/**
	 * push a string to the stack
	 */
	void PushString(const t_str& str);

	/**
	 * push data onto the stack
	 */
	void PushData(const t_data& data, VMType ty = VMType::UNKNOWN, bool err_on_unknown = true);


	/**
	 * read data from memory
	 */
	std::tuple<VMType, t_data> ReadMemData(t_addr addr);

	/**
	 * write data to memory
	 */
	void WriteMemData(t_addr addr, const t_data& data);


	/**
	 * read a raw value from memory
	 */
	template<class t_val>
	t_val ReadMemRaw(t_addr addr) const
	{
		// string type
		if constexpr(std::is_same_v<std::decay_t<t_val>, t_str>)
		{
			t_addr len = ReadMemRaw<t_addr>(addr);
			addr += m_addrsize;
			CheckMemoryBounds(addr, len);

			t_char* begin = reinterpret_cast<t_char*>(&m_mem[addr]);
			t_str str(begin, len);
			return str;
		}

		// primitive types
		else
		{
			CheckMemoryBounds(addr, sizeof(t_val));

			t_val val = *reinterpret_cast<t_val*>(&m_mem[addr]);
			return val;
		}
	}


	/**
	 * write a raw value to memory
	 */
	template<class t_val>
	void WriteMemRaw(t_addr addr, const t_val& val)
	{
		// string type
		if constexpr(std::is_same_v<std::decay_t<t_val>, t_str>)
		{
			t_addr len = static_cast<t_addr>(val.length());
			CheckMemoryBounds(addr, m_addrsize + len);

			// write string length
			WriteMemRaw<t_addr>(addr, len);
			addr += m_addrsize;

			// write string
			t_char* begin = reinterpret_cast<t_char*>(&m_mem[addr]);
			std::memcpy(begin, val.data(), len);
		}

		// primitive types
		else
		{
			CheckMemoryBounds(addr, sizeof(t_val));

			*reinterpret_cast<t_val*>(&m_mem[addr]) = val;
		}
	}


	/**
	 * get the value on top of the stack
	 */
	template<class t_val, t_addr valsize = sizeof(t_val)>
	t_val TopRaw(t_addr sp_offs = 0) const
	{
		t_addr addr = m_sp + sp_offs;
		CheckMemoryBounds(addr, valsize);
		return *reinterpret_cast<t_val*>(m_mem.get() + addr);
	}


	/**
	 * pop a raw value from the stack
	 */
	template<class t_val, t_addr valsize = sizeof(t_val)>
	t_val PopRaw()
	{
		CheckMemoryBounds(m_sp, valsize);

		t_val val = *reinterpret_cast<t_val*>(m_mem.get() + m_sp);
		m_sp += valsize;	// stack grows to lower addresses
		return val;
	}


	/**
	 * push a raw value onto the stack
	 */
	template<class t_val, t_addr valsize = sizeof(t_val)>
	void PushRaw(const t_val& val)
	{
		CheckMemoryBounds(m_sp, valsize);

		m_sp -= valsize;	// stack grows to lower addresses
		*reinterpret_cast<t_val*>(m_mem.get() + m_sp) = val;
	}


	/**
	 * cast from one variable type to the other
	 */
	template<class t_to>
	void OpCast()
	{
		t_data data = TopData();

		if(std::holds_alternative<t_real>(data))
		{
			if constexpr(std::is_same_v<std::decay_t<t_to>, t_real>)
				return;  // don't need to cast to the same type

			t_real val = std::get<t_real>(data);

			// convert to string
			if constexpr(std::is_same_v<std::decay_t<t_to>, t_str>)
			{
				std::ostringstream ostr;
				ostr << val;
				PopData();
				PushData(ostr.str());
			}

			// convert to primitive type
			else
			{
				PopData();
				PushData(static_cast<t_to>(val));
			}
		}
		else if(std::holds_alternative<t_int>(data))
		{
			if constexpr(std::is_same_v<std::decay_t<t_to>, t_int>)
				return;  // don't need to cast to the same type

			t_int val = std::get<t_int>(data);

			// convert to string
			if constexpr(std::is_same_v<std::decay_t<t_to>, t_str>)
			{
				std::ostringstream ostr;
				ostr << val;
				PopData();
				PushData(ostr.str());
			}

			// convert to primitive type
			else
			{
				PopData();
				PushData(static_cast<t_to>(val));
			}
		}
		else if(std::holds_alternative<t_str>(data))
		{
			if constexpr(std::is_same_v<std::decay_t<t_to>, t_str>)
				return;  // don't need to cast to the same type

			const t_str& val = std::get<t_str>(data);

			t_to conv_val{};
			std::istringstream{val} >> conv_val;
			PopData();
			PushData(conv_val);
		}
	}


	/**
	 * arithmetic operation
	 */
	template<class t_val, char op>
	t_val OpArithmetic(const t_val& val1, const t_val& val2)
	{
		t_val result{};

		// string operators
		if constexpr(std::is_same_v<std::decay_t<t_val>, t_str>)
		{
			if constexpr(op == '+')
				result = val1 + val2;
		}

		// int / real operators
		else
		{
			if constexpr(op == '+')
				result = val1 + val2;
			else if constexpr(op == '-')
				result = val1 - val2;
			else if constexpr(op == '*')
				result = val1 * val2;
			else if constexpr(op == '/')
				result = val1 / val2;
			else if constexpr(op == '%' && std::is_integral_v<t_val>)
				result = val1 % val2;
			else if constexpr(op == '%' && std::is_floating_point_v<t_val>)
				result = std::fmod(val1, val2);
			else if constexpr(op == '^' && std::is_floating_point_v<t_val>)
				result = pow<t_val>(val1, val2);
		}

		return result;
	}


	/**
	 * arithmetic operation
	 */
	template<char op>
	void OpArithmetic()
	{
		t_data val2 = PopData();
		t_data val1 = PopData();

		bool is_real = false;
		bool is_int = false;
		bool is_str = false;

		if(std::holds_alternative<t_real>(val1) &&
			std::holds_alternative<t_real>(val2))
			is_real = true;
		else if(std::holds_alternative<t_int>(val1) &&
			std::holds_alternative<t_int>(val2))
			is_int = true;
		else if(std::holds_alternative<t_str>(val1) &&
			std::holds_alternative<t_str>(val2))
			is_str = true;

		t_data result;

		if(is_real)
		{
			result = OpArithmetic<t_real, op>(
				std::get<t_real>(val1), std::get<t_real>(val2));
		}
		else if(is_int)
		{
			result = OpArithmetic<t_int, op>(
				std::get<t_int>(val1), std::get<t_int>(val2));
		}
		else if(is_str)
		{
			result = OpArithmetic<t_str, op>(
				std::get<t_str>(val1), std::get<t_str>(val2));
		}
		else
		{
			throw std::runtime_error("Type mismatch in arithmetic operation.");
		}

		PushData(result);
	}


	/**
	 * logical operation
	 */
	template<char op>
	void OpLogical()
	{
		// might also use PopData and PushData in case ints
		// should also be allowed in boolean expressions
		t_bool val2 = PopRaw<t_bool, m_boolsize>();
		t_bool val1 = PopRaw<t_bool, m_boolsize>();

		t_bool result = 0;

		if constexpr(op == '&')
			result = val1 && val2;
		else if constexpr(op == '|')
			result = val1 || val2;
		else if constexpr(op == '^')
			result = val1 ^ val2;

		PushRaw<t_bool, m_boolsize>(result);
	}


	/**
	 * binary operation
	 */
	template<class t_val, char op>
	t_val OpBinary(const t_val& val1, const t_val& val2)
	{
		t_val result{};

		// int operators
		if constexpr(std::is_same_v<std::decay_t<t_int>, t_int>)
		{
			if constexpr(op == '&')
				result = val1 & val2;
			else if constexpr(op == '|')
				result = val1 | val2;
			else if constexpr(op == '^')
				result = val1 ^ val2;
			else if constexpr(op == '<')  // left shift
				result = val1 << val2;
			else if constexpr(op == '>')  // right shift
				result = val1 >> val2;
			else if constexpr(op == 'l')  // left rotation
				result = static_cast<t_int>(std::rotl<t_uint>(val1, static_cast<int>(val2)));
			else if constexpr(op == 'r')  // right rotation
				result = static_cast<t_int>(std::rotr<t_uint>(val1, static_cast<int>(val2)));
		}

		return result;
	}


	/**
	 * binary operation
	 */
	template<char op>
	void OpBinary()
	{
		t_data val2 = PopData();
		t_data val1 = PopData();

		bool is_int = false;

		if(std::holds_alternative<t_int>(val1) &&
			std::holds_alternative<t_int>(val2))
			is_int = true;

		t_data result;

		if(is_int)
		{
			result = OpBinary<t_int, op>(
				std::get<t_int>(val1), std::get<t_int>(val2));
		}
		else
		{
			throw std::runtime_error("Type mismatch in binary operation.");
		}

		PushData(result);
	}


	/**
	 * comparison operation
	 */
	template<class t_val, OpCode op>
	t_bool OpComparison(const t_val& val1, const t_val& val2)
	{
		t_bool result = 0;
		// TODO: include an epsilon region for float comparisons

		// string comparison
		if constexpr(std::is_same_v<std::decay_t<t_val>, t_str>)
		{
			if constexpr(op == OpCode::EQU)
				result = (val1 == val2);
			else if constexpr(op == OpCode::NEQU)
				result = (val1 != val2);
		}

		// integer /  real comparison
		else
		{
			if constexpr(op == OpCode::GT)
				result = (val1 > val2);
			else if constexpr(op == OpCode::LT)
				result = (val1 < val2);
			else if constexpr(op == OpCode::GEQU)
				result = (val1 >= val2);
			else if constexpr(op == OpCode::LEQU)
				result = (val1 <= val2);
			else if constexpr(op == OpCode::EQU)
				result = (val1 == val2);
			else if constexpr(op == OpCode::NEQU)
				result = (val1 != val2);
		}

		return result;
	}


	/**
	 * comparison operation
	 */
	template<OpCode op>
	void OpComparison()
	{
		t_data val2 = PopData();
		t_data val1 = PopData();

		bool is_real = false;
		bool is_int = false;
		bool is_str = false;

		if(std::holds_alternative<t_real>(val1) &&
			std::holds_alternative<t_real>(val2))
			is_real = true;
		else if(std::holds_alternative<t_int>(val1) &&
			std::holds_alternative<t_int>(val2))
			is_int = true;
		else if(std::holds_alternative<t_str>(val1) &&
			std::holds_alternative<t_str>(val2))
			is_str = true;

		t_bool result;

		if(is_real)
		{
			result = OpComparison<t_real, op>(
				std::get<t_real>(val1), std::get<t_real>(val2));
		}
		else if(is_int)
		{
			result = OpComparison<t_int, op>(
				std::get<t_int>(val1), std::get<t_int>(val2));
		}
		else if(is_str)
		{
			result = OpComparison<t_str, op>(
				std::get<t_str>(val1), std::get<t_str>(val2));
		}
		else
		{
			throw std::runtime_error("Type mismatch in comparison operation.");
		}

		PushRaw<t_bool, m_boolsize>(result);
	}


private:
	void CheckMemoryBounds(t_addr addr, std::size_t size = 1) const
	{
		if(std::size_t(addr) + size > std::size_t(m_memsize) || addr < 0)
			throw std::runtime_error("Tried to access out of memory bounds.");
	}


private:
	bool m_debug{false};
	std::unique_ptr<t_byte[]> m_mem{};

	// registers
	t_addr m_ip{};	// instruction pointer
	t_addr m_sp{};	// stack pointer
	t_addr m_bp{};  // base pointer for local variables
	t_addr m_gbp{}; // global base pointer
};


#endif
