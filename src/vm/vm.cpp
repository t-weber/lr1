/**
 * zero-address code vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 8-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "vm.h"

#include <iostream>
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

			// push direct read data onto stack
			case OpCode::PUSHF:
			{
				// get data value from memory
				t_real val = *reinterpret_cast<t_real*>(&m_mem[m_ip]);
				m_ip += m_realsize;

				//std::cout << "push " << val << std::endl;
				Push<t_real, m_realsize>(val);
				break;
			}

			// push direct int data onto stack
			case OpCode::PUSHI:
			{
				// get data value from memory
				t_int val = *reinterpret_cast<t_int*>(&m_mem[m_ip]);
				m_ip += m_intsize;

				//std::cout << "push " << val << std::endl;
				Push<t_int, m_intsize>(val);
				break;
			}

			// push direct address onto stack
			case OpCode::PUSHADDR:
			{
				// get data value from memory
				t_addr val = *reinterpret_cast<t_addr*>(&m_mem[m_ip]);
				m_ip += m_addrsize;

				//std::cout << "push " << val << std::endl;
				Push<t_addr, m_addrsize>(val);
				break;
			}

			case OpCode::MOVREGF:
			{
				// first byte: register info
				t_byte regval = *reinterpret_cast<t_byte*>(&m_mem[m_ip]);
				m_ip += m_bytesize;

				// address (offset)
				//t_addr addr = *reinterpret_cast<t_addr*>(&m_mem[m_ip]);
				//m_ip += m_addrsize;
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
				if(regval & 0b10000000)	// to address in register
				{
					// pop data and write it to memory
					t_real val = Pop<t_real, m_realsize>();
					*reinterpret_cast<t_real*>(&m_mem[addr]) = val;

					//std::cout << "Wrote " << val << " to 0x" << std::hex << addr << std::endl;
				}
				else	// from address in register
				{
					// read and push data from memory
					t_real val = *reinterpret_cast<t_real*>(&m_mem[addr]);
					Push<t_real, m_realsize>(val);
				}

				break;
			}

			case OpCode::MOVREGI:
			{
				break;
			}

			// pop an address and push the value it points to
			case OpCode::DEREFF:
			{
				t_addr addr = Pop<t_addr, m_addrsize>();
				t_real val = *reinterpret_cast<t_real*>(&m_mem[m_bp + addr]);
				Push<t_real, m_realsize>(val);
				break;
			}

			// pop an address and push the value it points to
			case OpCode::DEREFI:
			{
				t_addr addr = Pop<t_addr, m_addrsize>();
				t_int val = *reinterpret_cast<t_int*>(&m_mem[m_bp + addr]);
				Push<t_int, m_intsize>(val);
				break;
			}

			case OpCode::UADDF:
			{
				break;
			}

			case OpCode::USUBF:
			{
				t_real val = Pop<t_real, m_realsize>();

				//std::cout << " - " << val << std::endl;
				Push<t_real, m_realsize>(-val);
				break;
			}

			case OpCode::ADDF:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " + " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 + val2);
				break;
			}

			case OpCode::SUBF:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " - " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 - val2);
				break;
			}

			case OpCode::MULF:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " * " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 * val2);
				break;
			}

			case OpCode::DIVF:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " / " << val2 << std::endl;
				Push<t_real, m_realsize>(val1 / val2);
				break;
			}

			case OpCode::MODF:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " % " << val2 << std::endl;
				Push<t_real, m_realsize>(std::fmod(val1, val2));
				break;
			}

			case OpCode::POWF:
			{
				t_real val2 = Pop<t_real, m_realsize>();
				t_real val1 = Pop<t_real, m_realsize>();

				//std::cout << val1 << " ^ " << val2 << std::endl;
				Push<t_real, m_realsize>(std::pow(val1, val2));
				break;
			}

			case OpCode::UADDI:
			{
				break;
			}

			case OpCode::USUBI:
			{
				t_int val = Pop<t_int, m_intsize>();

				//std::cout << " - " << val << std::endl;
				Push<t_int, m_intsize>(-val);
				break;
			}

			case OpCode::ADDI:
			{
				t_int val2 = Pop<t_int, m_intsize>();
				t_int val1 = Pop<t_int, m_intsize>();

				//std::cout << val1 << " + " << val2 << std::endl;
				Push<t_int, m_intsize>(val1 + val2);
				break;
			}

			case OpCode::SUBI:
			{
				t_int val2 = Pop<t_int, m_intsize>();
				t_int val1 = Pop<t_int, m_intsize>();

				//std::cout << val1 << " - " << val2 << std::endl;
				Push<t_int, m_intsize>(val1 - val2);
				break;
			}

			case OpCode::MULI:
			{
				t_int val2 = Pop<t_int, m_intsize>();
				t_int val1 = Pop<t_int, m_intsize>();

				//std::cout << val1 << " * " << val2 << std::endl;
				Push<t_int, m_intsize>(val1 * val2);
				break;
			}

			case OpCode::DIVI:
			{
				t_int val2 = Pop<t_int, m_intsize>();
				t_int val1 = Pop<t_int, m_intsize>();

				//std::cout << val1 << " / " << val2 << std::endl;
				Push<t_int, m_intsize>(val1 / val2);
				break;
			}

			case OpCode::MODI:
			{
				t_int val2 = Pop<t_int, m_intsize>();
				t_int val1 = Pop<t_int, m_intsize>();

				//std::cout << val1 << " % " << val2 << std::endl;
				Push<t_int, m_intsize>(val1 % val2);
				break;
			}

			case OpCode::POWI:
			{
				t_int val2 = Pop<t_int, m_intsize>();
				t_int val1 = Pop<t_int, m_intsize>();

				//std::cout << val1 << " ^ " << val2 << std::endl;
				Push<t_int, m_intsize>(std::pow(val1, val2));
				break;
			}

			case OpCode::FTOI: // converts t_real to t_int
			{
				t_real val = Pop<t_real, m_realsize>();
				Push<t_int, m_intsize>(static_cast<t_int>(val));
				break;
			}

			case OpCode::ITOF: // converts t_int to t_real
			{
				t_int val = Pop<t_int, m_intsize>();
				Push<t_real, m_realsize>(static_cast<t_real>(val));
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
