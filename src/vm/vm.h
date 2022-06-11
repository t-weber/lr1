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
#include <cmath>
#include <iostream>

#include "../codegen/lval.h"
#include "opcodes.h"
#include "helpers.h"


class VM
{
public:
	// data types
	using t_byte = ::t_vm_byte;
	using t_addr = ::t_vm_addr;
	using t_real = ::t_real;
	using t_int = ::t_int;
	using t_bool = t_byte;

	// variant of all data types
	using t_data = std::variant<t_real, t_int, t_bool, t_addr>;

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

	void Reset();
	bool Run();

	void SetMem(t_addr addr, t_byte data);
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
	 * get the value on top of the stack
	 */
	template<class t_val>
	t_val Top() const
	{
		t_addr addr = m_sp + 1; // skip descriptor byte
		return *reinterpret_cast<t_val*>(m_mem.get() + addr);
	}


protected:
	t_addr GetDataSize(const t_data& data) const;


	/**
	 * pop an address from the stack
	 */
	t_addr PopAddress();


	/**
	 * pop data from the stack
	 */
	t_data PopData();

	/**
	 * push data onto the stack
	 */
	void PushData(const t_data& data);


	/**
	 * read data from memory
	 */
	t_data ReadMemData(t_addr addr);

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
		t_val val = *reinterpret_cast<t_val*>(&m_mem[addr]);
		return val;
	}


	/**
	 * write a raw value to memory
	 */
	template<class t_val>
	void WriteMemRaw(t_addr addr, t_val val)
	{
		*reinterpret_cast<t_val*>(&m_mem[addr]) = val;
	}


	/**
	 * pop a raw value from the stack
	 */
	template<class t_val, t_addr valsize = sizeof(t_val)>
	t_val PopRaw()
	{
		t_val val = *reinterpret_cast<t_val*>(m_mem.get() + m_sp);
		m_sp += valsize;	// stack grows to lower addresses
		return val;
	}


	/**
	 * push a raw value onto the stack
	 */
	template<class t_val, t_addr valsize = sizeof(t_val)>
	void PushRaw(t_val val)
	{
		m_sp -= valsize;	// stack grows to lower addresses
		*reinterpret_cast<t_val*>(m_mem.get() + m_sp) = val;
	}


	/**
	 * cast from one variable type to the other
	 */
	template<class t_to, t_addr to_size>
	void OpCast()
	{
		t_data val = PopData();
		if(std::holds_alternative<t_real>(val))
			PushData(static_cast<t_to>(std::get<t_real>(val)));
		else if(std::holds_alternative<t_int>(val))
			PushData(static_cast<t_to>(std::get<t_int>(val)));
	}


	/**
	 * arithmetic operation
	 */
	template<class t_val, char op>
	t_val OpArithmetic(const t_val& val1, const t_val& val2)
	{
		t_val result{};

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

		if(std::holds_alternative<t_real>(val1) &&
			std::holds_alternative<t_real>(val2))
			is_real = true;
		else if(std::holds_alternative<t_int>(val1) &&
			std::holds_alternative<t_int>(val2))
			is_int = true;

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
	 * comparison operation
	 */
	template<class t_val, OpCode op>
	t_bool OpComparison(const t_val& val1, const t_val& val2)
	{
		t_bool result = 0;
		// TODO: include an epsilon region for float comparisons

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

		if(std::holds_alternative<t_real>(val1) &&
			std::holds_alternative<t_real>(val2))
			is_real = true;
		else if(std::holds_alternative<t_int>(val1) &&
			std::holds_alternative<t_int>(val2))
			is_int = true;

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
		else
		{
			throw std::runtime_error("Type mismatch in comparison operation.");
		}

		PushRaw<t_bool, m_boolsize>(result);
	}


private:
	std::unique_ptr<t_byte[]> m_mem{};

	// registers
	t_addr m_ip{};	// instruction pointer
	t_addr m_sp{};	// stack pointer
	t_addr m_bp{};  // base pointer for local variables
	t_addr m_gbp{}; // global base pointer
};


#endif
