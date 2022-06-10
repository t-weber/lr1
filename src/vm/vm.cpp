/**
 * zero-address code vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 8-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "vm.h"

#include <iostream>
#include <cstring>


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

			case OpCode::NOP:
			{
				break;
			}

			// push direct real data onto stack
			case OpCode::PUSHF:
			{
				OpPush<t_real, m_realsize>();
				break;
			}

			// push direct int data onto stack
			case OpCode::PUSHI:
			{
				OpPush<t_int, m_intsize>();
				break;
			}

			// push direct address onto stack
			case OpCode::PUSHADDR:
			{
				// get data value from memory
				t_addr val = ReadMem<t_addr>(m_ip);
				m_ip += m_addrsize;

				//std::cout << "push " << val << std::endl;
				Push<t_addr, m_addrsize>(val);
				break;
			}

			case OpCode::MOVREGF:
			{
				OpMovReg<t_real, m_realsize>();
				break;
			}

			case OpCode::MOVREGI:
			{
				OpMovReg<t_int, m_intsize>();
				break;
			}

			// pop an address and push the value it points to
			case OpCode::DEREFF:
			{
				OpDeref<t_real, m_realsize>();
				break;
			}

			// pop an address and push the value it points to
			case OpCode::DEREFI:
			{
				OpDeref<t_int, m_intsize>();
				break;
			}

			case OpCode::USUBF:
			{
				OpUMinus<t_real, m_realsize>();
				break;
			}

			case OpCode::ADDF:
			{
				OpArithmetic<t_real, m_realsize, '+'>();
				break;
			}

			case OpCode::SUBF:
			{
				OpArithmetic<t_real, m_realsize, '-'>();
				break;
			}

			case OpCode::MULF:
			{
				OpArithmetic<t_real, m_realsize, '*'>();
				break;
			}

			case OpCode::DIVF:
			{
				OpArithmetic<t_real, m_realsize, '/'>();
				break;
			}

			case OpCode::MODF:
			{
				OpArithmetic<t_real, m_realsize, '%'>();
				break;
			}

			case OpCode::POWF:
			{
				OpArithmetic<t_real, m_realsize, '^'>();
				break;
			}

			case OpCode::USUBI:
			{
				OpUMinus<t_int, m_intsize>();
				break;
			}

			case OpCode::ADDI:
			{
				OpArithmetic<t_int, m_intsize, '+'>();
				break;
			}

			case OpCode::SUBI:
			{
				OpArithmetic<t_int, m_intsize, '-'>();
				break;
			}

			case OpCode::MULI:
			{
				OpArithmetic<t_int, m_intsize, '*'>();
				break;
			}

			case OpCode::DIVI:
			{
				OpArithmetic<t_int, m_intsize, '/'>();
				break;
			}

			case OpCode::MODI:
			{
				OpArithmetic<t_int, m_intsize, '%'>();
				break;
			}

			case OpCode::POWI:
			{
				OpArithmetic<t_int, m_intsize, '^'>();
				break;
			}

			case OpCode::FTOI: // converts t_real to t_int
			{
				OpCast<t_int, t_real, m_intsize, m_realsize>();
				break;
			}

			case OpCode::ITOF: // converts t_int to t_real
			{
				OpCast<t_real, t_int, m_realsize, m_intsize>();
				break;
			}

			default:
			{
				std::cerr << "Error: Invalid opcode " << std::hex
					<< static_cast<t_addr>(_op) << std::endl;
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
	m_bp = m_sp - m_framesize;

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
