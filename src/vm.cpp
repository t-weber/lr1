/**
 * zero-address code vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 8-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "vm.h"

#include <cstring>
#include <cmath>


VM::VM(t_addr memsize) : m_memsize{memsize}
{
	m_mem.reset(new t_byte[m_memsize]);
	Reset();
}


bool VM::Run()
{
	bool running = true;

	while(running)
	{
		t_byte _op = m_mem[m_ip++];
		OpCode op = static_cast<OpCode>(_op);

		switch(op)
		{
			case OpCode::HALT:
			{
				running = false;
				break;
			}

			case OpCode::PUSH:
			{
				// get data value from memory
				t_real val = *reinterpret_cast<t_real*>(&m_mem[m_ip]);
				m_ip += m_realsize;

				//std::cout << "push " << val << std::endl;
				Push<t_real, m_realsize>(val);
				break;
			}

			case OpCode::UADD:
			{
				break;
			}

			case OpCode::USUB:
			{
				t_real val = Pop<t_real, m_realsize>();

				//std::cout << " - " << val << std::endl;
				Push<t_real, m_realsize>(-val);
				break;
			}

			case OpCode::ADD:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " + " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 + val2);
				break;
			}

			case OpCode::SUB:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " - " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 - val2);
				break;
			}

			case OpCode::MUL:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " * " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 * val2);
				break;
			}

			case OpCode::DIV:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " / " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 / val2);
				break;
			}

			case OpCode::MOD:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " % " << val2 << std::endl;
				Push<t_real, m_realsize>(std::fmod(val1, val2));
				break;
			}

			case OpCode::POW:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " ^ " << val2 << std::endl;
				Push<t_real, m_realsize>(std::pow(val1, val2));
				break;
			}

			default:
			{
				std::cerr << "Error: Invalid opcode " << std::hex << _op << std::endl;
				return false;
			}
		}

		// wrap around
		if(m_ip > m_memsize)
			m_ip %= m_memsize;
	}

	return true;
}


void VM::Reset()
{
	m_ip = 0;
	m_sp = m_memsize;

	std::memset(m_mem.get(), 0, m_memsize);
}


void VM::SetMem(t_addr addr, t_byte data)
{
	m_mem[addr % m_memsize] = data;
}


void VM::SetMem(t_addr addr, const std::string& data)
{
	for(std::size_t i=0; i<data.size(); ++i)
		SetMem(addr + (t_addr)(i), static_cast<t_byte>(data[i]));
}
