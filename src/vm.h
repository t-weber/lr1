/**
 * zero-address code vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 8-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_0ACVM_H__
#define __LR1_0ACVM_H__

#include <memory>
#include <cstdint>

#include "ast_asm.h"
#include "lval.h"


class VM
{
public:
	// types and constants
	using t_byte = std::uint8_t;
	using t_addr = std::uint16_t;
	using t_real = ::t_real;
	static constexpr const t_addr m_bytesize = sizeof(t_byte);
	static constexpr const t_addr m_addrsize = sizeof(t_addr);
	static constexpr const t_addr m_realsize = sizeof(t_real);
	t_addr m_memsize = 1024;


public:
	VM(t_addr memsize = 1024);
	~VM() = default;

	void Reset();
	bool Run();

	void SetMem(t_addr addr, t_byte data);
	void SetMem(t_addr addr, const std::string& data);

	t_addr GetSP() const { return m_sp; }
	t_addr GetIP() const { return m_ip; }

	void SetSP(t_addr sp) { m_sp = sp; }
	void SetIP(t_addr ip) { m_ip = ip; }


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
	 * get the value on top of the stack
	 */
	template<class t_val> t_val Top() const
	{
		return *reinterpret_cast<t_val*>(m_mem.get() + m_sp);
	}


private:
	std::unique_ptr<t_byte[]> m_mem{};

	// registers
	t_addr m_ip{};	// instruction pointer
	t_addr m_sp{};	// stack pointer
};


#endif
