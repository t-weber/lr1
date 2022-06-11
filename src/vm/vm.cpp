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

			// push direct data onto stack
			case OpCode::PUSH:
			{
				t_data val = ReadMemData(m_ip);
				m_ip += GetDataSize(val) + m_bytesize;
				PushData(val);
				break;
			}

			// push direct address onto stack
			case OpCode::PUSHADDR:
			{
				// get address base register from memory
				t_byte regval = ReadMemRaw<t_byte>(m_ip);
				m_ip += m_bytesize;

				// get address from memory
				t_addr val = ReadMemRaw<t_addr>(m_ip);
				m_ip += m_addrsize;

				PushRaw<t_addr, m_addrsize>(val);
				PushRaw<t_byte, m_bytesize>(regval);
				break;
			}

			case OpCode::WRMEM:
			{
				// variable address
				t_addr addr = PopAddress();

				// pop data and write it to memory
				t_data val = PopData();
				WriteMemData(addr, val);
				break;
			}

			case OpCode::RDMEM:
			{
				// variable address
				t_addr addr = PopAddress();

				// read and push data from memory
				t_data val = ReadMemData(addr);
				PushData(val);
				break;
			}

			// pop an address and push the value it points to
			case OpCode::DEREF:
			{
				t_addr addr = PopAddress();
				t_data val = ReadMemData(addr);
				PushData(val);

				//std::cout << "dereferenced address " << addr << ": " << std::get<t_real>(val) << std::endl;
				break;
			}

			case OpCode::USUB:
			{
				t_data val = PopData();
				t_data result;

				if(std::holds_alternative<t_real>(val))
					result = -std::get<t_real>(val);
				else if(std::holds_alternative<t_int>(val))
					result = -std::get<t_int>(val);
				else
					throw std::runtime_error("Type mismatch in arithmetic operation.");

				PushData(result);
				break;
			}

			case OpCode::ADD:
			{
				OpArithmetic<'+'>();
				break;
			}

			case OpCode::SUB:
			{
				OpArithmetic<'-'>();
				break;
			}

			case OpCode::MUL:
			{
				OpArithmetic<'*'>();
				break;
			}

			case OpCode::DIV:
			{
				OpArithmetic<'/'>();
				break;
			}

			case OpCode::MOD:
			{
				OpArithmetic<'%'>();
				break;
			}

			case OpCode::POW:
			{
				OpArithmetic<'^'>();
				break;
			}

			case OpCode::AND:
			{
				OpLogical<'&'>();
				break;
			}

			case OpCode::OR:
			{
				OpLogical<'|'>();
				break;
			}

			case OpCode::XOR:
			{
				OpLogical<'^'>();
				break;
			}

			case OpCode::NOT:
			{
				// might also use PopData and PushData in case ints
				// should also be allowed in boolean expressions
				t_bool val = PopRaw<t_bool, m_boolsize>();
				PushRaw<t_bool, m_boolsize>(!val);
				break;
			}

			case OpCode::GT:
			{
				OpComparison<OpCode::GT>();
				break;
			}

			case OpCode::LT:
			{
				OpComparison<OpCode::LT>();
				break;
			}

			case OpCode::GEQU:
			{
				OpComparison<OpCode::GEQU>();
				break;
			}

			case OpCode::LEQU:
			{
				OpComparison<OpCode::LEQU>();
				break;
			}

			case OpCode::EQU:
			{
				OpComparison<OpCode::EQU>();
				break;
			}

			case OpCode::NEQU:
			{
				OpComparison<OpCode::NEQU>();
				break;
			}

			case OpCode::TOI: // converts value to t_int
			{
				OpCast<t_int, m_intsize>();
				break;
			}

			case OpCode::TOF: // converts value to t_real
			{
				OpCast<t_real, m_realsize>();
				break;
			}

			case OpCode::JMP: // jump to direct address
			{
				// get address from stack and set ip
				m_ip = PopAddress();
				break;
			}

			case OpCode::JMPCND: // conditional jump to direct address
			{
				// get address from stack
				t_addr addr = PopAddress();

				// get boolean condition result from stack
				t_bool cond = PopRaw<t_bool, m_boolsize>();

				// set instruction pointer
				if(cond)
					m_ip = addr;
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


/**
 * pop an address from the stack
 * an address consists of the index of an register
 * holding the base address and an offset address
 */
VM::t_addr VM::PopAddress()
{
	// get register info from stack
	t_byte regval = PopRaw<t_byte, m_bytesize>();

	// get address from stack
	t_addr addr = PopRaw<t_addr, m_addrsize>();

	// get absolute address using base address from register
	VMRegister thereg = static_cast<VMRegister>(regval);
	switch(thereg)
	{
		case VMRegister::MEM: break;
		case VMRegister::IP: addr += m_ip; break;
		case VMRegister::SP: addr += m_sp; break;
		case VMRegister::BP: addr += m_bp; break;
		case VMRegister::GBP: addr += m_gbp; break;
	}

	return addr;
}


/**
 * pop data from the stack, which is prefixed
 * with a type descriptor byte
 */
VM::t_data VM::PopData()
{
	// get data type info from stack
	t_byte tyval = PopRaw<t_byte, m_bytesize>();
	VMType ty = static_cast<VMType>(tyval);

	t_data dat;

	switch(ty)
	{
		case VMType::REAL:
		{
			t_real val = PopRaw<t_real, m_realsize>();
			std::get<t_real>(dat) = val;
			break;
		}
		case VMType::INT:
		{
			t_int val = PopRaw<t_int, m_intsize>();
			std::get<t_int>(dat) = val;
			break;
		}

		// TODO: handle pointer types

		default:
		{
			throw std::runtime_error("Pop: Data type not yet implemented");
			break;
		}
	}

	return dat;
}


/**
 * push the raw data followed by a data type descriptor
 */
void VM::PushData(const VM::t_data& data)
{
	if(std::holds_alternative<t_real>(data))
	{
		// push the actual data
		PushRaw<t_real, m_realsize>(std::get<t_real>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(VMType::REAL));
	}
	else if(std::holds_alternative<t_real>(data))
	{
		// push the actual data
		PushRaw<t_int, m_intsize>(std::get<t_int>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(VMType::INT));
	}
	else
	{
		throw std::runtime_error("Push: Data type not yet implemented");
	}
}


/**
 * read type-prefixed data from memory
 */
VM::t_data VM::ReadMemData(VM::t_addr addr)
{
	// get data type info from memory
	t_byte tyval = ReadMemRaw<t_byte>(addr);
	addr += m_bytesize;
	VMType ty = static_cast<VMType>(tyval);

	//std::cout << "read data type " << int(tyval) << " from address " << (addr-1) << std::endl;
	t_data dat;
	switch(ty)
	{
		case VMType::REAL:
		{
			t_real val = ReadMemRaw<t_real>(addr);
			std::get<t_real>(dat) = val;
			break;
		}
		case VMType::INT:
		{
			t_int val = ReadMemRaw<t_int>(addr);
			std::get<t_int>(dat) = val;
			break;
		}

		// TODO: handle pointer types

		default:
		{
			throw std::runtime_error("ReadMem: Data type not yet implemented");
			break;
		}
	}

	return dat;
}


/**
 * write type-prefixed data to memory
 */
void VM::WriteMemData(VM::t_addr addr, const VM::t_data& data)
{
	if(std::holds_alternative<t_real>(data))
	{
		//std::cout << "writing real value " << std::get<t_real>(data) << " to address " << addr << std::endl;

		// write descriptor prefix
		WriteMemRaw<t_byte>(addr, static_cast<t_byte>(VMType::REAL));
		addr += m_bytesize;

		// write the actual data
		WriteMemRaw<t_real>(addr, std::get<t_real>(data));
	}
	else if(std::holds_alternative<t_int>(data))
	{
		//std::cout << "writing int value " << std::get<t_int>(data) << " to address " << addr << std::endl;

		// write descriptor prefix
		WriteMemRaw<t_byte>(addr, static_cast<t_byte>(VMType::INT));
		addr += m_bytesize;

		// write the actual data
		WriteMemRaw<t_int>(addr, std::get<t_int>(data));
	}
	else
	{
		throw std::runtime_error("WriteMem: Data type not yet implemented");
	}
}


VM::t_addr VM::GetDataSize(const t_data& data) const
{
	if(std::holds_alternative<t_real>(data))
		return m_realsize;
	else if(std::holds_alternative<t_int>(data))
		return m_intsize;

	return 0;
}


void VM::Reset()
{
	m_ip = 0;
	m_sp = m_memsize;
	m_bp = m_sp - m_framesize;
	m_gbp = m_bp;

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
