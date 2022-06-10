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
#include <cmath>

#include "../codegen/lval.h"
#include "opcodes.h"


class VM
{
public:
	// types and constants
	using t_byte = ::t_vm_byte;
	using t_addr = ::t_vm_addr;
	using t_real = ::t_real;
	using t_int = ::t_int;

	// data type sizes
	static constexpr const t_addr m_bytesize = sizeof(t_byte);
	static constexpr const t_addr m_addrsize = sizeof(t_addr);
	static constexpr const t_addr m_realsize = sizeof(t_real);
	static constexpr const t_addr m_intsize = sizeof(t_int);

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
	t_addr GetIP() const { return m_ip; }

	void SetSP(t_addr sp) { m_sp = sp; }
	void SetBP(t_addr bp) { m_bp = bp; }
	void SetIP(t_addr ip) { m_ip = ip; }


	/**
	 * get the value on top of the stack
	 */
	template<class t_val>
	t_val Top() const
	{
		return *reinterpret_cast<t_val*>(m_mem.get() + m_sp);
	}


protected:
	/**
	 * read a value from memory
	 */
	template<class t_val>
	t_val ReadMem(t_addr addr) const
	{
		t_val val = *reinterpret_cast<t_val*>(&m_mem[addr]);
		return val;
	}


	/**
	 * write a value to memory
	 */
	template<class t_val>
	void WriteMem(t_addr addr, t_val val)
	{
		*reinterpret_cast<t_real*>(&m_mem[addr]) = val;
	}


	/**
	 * pop a value from the stack
	 */
	template<class t_val, t_addr valsize = sizeof(t_val)>
	t_val Pop()
	{
		t_val val = *reinterpret_cast<t_val*>(m_mem.get() + m_sp);
		m_sp += valsize;	// stack grows to lower addresses
		return val;
	}


	/**
	 * push a value onto the stack
	 */
	template<class t_val, t_addr valsize = sizeof(t_val)>
	void Push(t_val val)
	{
		m_sp -= valsize;	// stack grows to lower addresses
		*reinterpret_cast<t_val*>(m_mem.get() + m_sp) = val;
	}


	/**
	 * push direct data onto stack
	 */
	template<class t_val, t_addr valsize>
	void OpPush()
	{
		// get data value from memory
		t_real val = ReadMem<t_val>(m_ip);
		m_ip += valsize;

		//std::cout << "push " << val << std::endl;
		Push<t_val, valsize>(val);
	}


	/**
	 * pop an address and push the value it points to
	 */
	template<class t_val, t_addr valsize>
	void OpDeref()
	{
		t_addr addr = Pop<t_addr, m_addrsize>();
		t_val val = ReadMem<t_val>(m_bp + addr);
		Push<t_val, valsize>(val);
	}


	template<class t_val, t_addr valsize>
	void OpMovReg()
	{
		// first byte: register info
		t_byte regval = ReadMem<t_byte>(m_ip);
		m_ip += m_bytesize;

		t_addr addr = Pop<t_addr, m_addrsize>();

		// get absolute address using base address from register
		Register thereg = static_cast<Register>(regval & 0b01111111);
		switch(thereg)
		{
			case Register::MEM: break;
			case Register::IP: addr += m_ip; break;
			case Register::SP: addr += m_sp; break;
			case Register::BP: addr += m_bp; break;
		}

		// first bit: direction
		if(regval & 0b10000000) // to address in register
		{
			// pop data and write it to memory
			t_val val = Pop<t_val, valsize>();
			WriteMem<t_val>(addr, val);

			//std::cout << "Wrote " << val << " to 0x" << std::hex << addr << std::endl;
		}
		else    // from address in register
		{
			// read and push data from memory
			t_val val = ReadMem<t_val>(addr);
			Push<t_val, valsize>(val);
		}
	}


	/**
	 * cast from one variable type to the other
	 */
	template<class t_to, class t_from, t_addr to_size, t_addr from_size>
	void OpCast()
	{
		t_from val = Pop<t_from, from_size>();
		Push<t_to, to_size>(static_cast<t_to>(val));
	}


	/**
	 * arithmetic operation
	 */
	template<class t_val, t_addr valsize, char op>
	void OpArithmetic()
	{
		t_val val2 = Pop<t_val, valsize>();
		t_val val1 = Pop<t_val, valsize>();

		t_val result;
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
		else if constexpr(op == '^')
			result = std::pow<t_val>(val1, val2);

		//std::cout << val1 << " + " << val2 << " = " << result << std::endl;
		Push<t_val, m_realsize>(result);
	}


	/**
	 * unary minus
	 */
	template<class t_val, t_addr valsize>
	void OpUMinus()
	{
		t_val val = Pop<t_val, valsize>();

		//std::cout << " - " << val << std::endl;
		Push<t_val, valsize>(-val);
	}


private:
	std::unique_ptr<t_byte[]> m_mem{};

	// registers
	t_addr m_ip{};	// instruction pointer
	t_addr m_sp{};	// stack pointer
	t_addr m_bp{};  // base pointer for local variables
};


#endif
