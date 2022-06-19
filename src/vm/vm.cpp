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
		//std::cout << "ip = " << m_ip << ", opcode: " << std::hex << static_cast<std::size_t>(op) << std::endl;

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
				auto [ty, val] = ReadMemData(m_ip);
				m_ip += GetDataSize(val) + m_bytesize;
				PushData(val, ty);
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
				auto [ty, val] = ReadMemData(addr);
				PushData(val, ty);
				break;
			}

			// pop an address and push the value it points to
			case OpCode::DEREF:
			{
				t_addr addr = PopAddress();
				auto [ty, val] = ReadMemData(addr);
				PushData(val, ty);

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

			case OpCode::CALL:
			{
				t_addr funcaddr = PopAddress();

				// save instruction and base pointer and
				// set up the function's stack frame for local variables
				PushAddress(m_ip, VMType::ADDR_MEM);
				PushAddress(m_bp, VMType::ADDR_MEM);

				//std::cout << "saved base pointer " << m_bp << std::endl;
				m_bp = m_sp;
				m_sp -= m_framesize;

				// jump to function
				m_ip = funcaddr;
				//std::cout << "calling function " << funcaddr << std::endl;
				break;
			}

			case OpCode::RET:
			{
				// get number of function arguments
				t_int num_args = std::get<t_int>(PopData());

				// if there's still a value on the stack, use it as return value
				t_data retval;
				if(m_sp + m_framesize < m_bp)
					retval = PopData();

				// remove the function's stack frame
				m_sp = m_bp;

				m_bp = PopAddress();
				m_ip = PopAddress();  // jump back
				//std::cout << "restored base pointer " << m_bp << std::endl;

				// remove function arguments from stack
				for(t_int arg=0; arg<num_args; ++arg)
					PopData();

				PushData(retval, VMType::UNKNOWN, false);
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
	// get register/type info from stack
	t_byte regval = PopRaw<t_byte, m_bytesize>();

	// get address from stack
	t_addr addr = PopRaw<t_addr, m_addrsize>();

	//std::cout << std::dec << "popped address " << addr << " of type " << int(regval) << std::endl;

	// get absolute address using base address from register
	VMType thereg = static_cast<VMType>(regval);
	switch(thereg)
	{
		case VMType::ADDR_MEM: break;
		case VMType::ADDR_IP: addr += m_ip; break;
		case VMType::ADDR_SP: addr += m_sp; break;
		case VMType::ADDR_BP: addr += m_bp; break;
		case VMType::ADDR_GBP: addr += m_gbp; break;
		default: throw std::runtime_error("Unknown address base register"); break;
	}

	//std::cout << "got address " << t_int(addr) << " from stack" << std::endl;
	return addr;
}


/**
 * push an address to stack
 */
void VM::PushAddress(t_addr addr, VMType ty)
{
	PushRaw<t_addr, m_addrsize>(addr);
	PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(ty));
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
			dat = val;
			break;
		}

		case VMType::ADDR_MEM:
		case VMType::ADDR_IP:
		case VMType::ADDR_SP:
		case VMType::ADDR_BP:
		case VMType::ADDR_GBP:
		{
			t_addr val = PopRaw<t_addr, m_addrsize>();
			dat = val;
			break;
		}

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
void VM::PushData(const VM::t_data& data, VMType ty, bool err_on_unknown)
{
	if(std::holds_alternative<t_real>(data))
	{
		//std::cout << "pushing real " << std::get<t_real>(data) << std::endl;

		// push the actual data
		PushRaw<t_real, m_realsize>(std::get<t_real>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(VMType::REAL));
	}
	else if(std::holds_alternative<t_int>(data))
	{
		//std::cout << "pushing int " << std::get<t_int>(data) << std::endl;

		// push the actual data
		PushRaw<t_int, m_intsize>(std::get<t_int>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(VMType::INT));
	}
	else if(std::holds_alternative<t_addr>(data))
	{
		//std::cout << "pushing address " << std::get<T_ADDR>(data) << std::endl;

		// push the actual address
		PushRaw<t_addr, m_addrsize>(std::get<T_ADDR>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(ty));
	}
	else if(err_on_unknown)
	{
		throw std::runtime_error("Push: Data type not yet implemented");
	}
}


/**
 * read type-prefixed data from memory
 */
std::tuple<VMType, VM::t_data> VM::ReadMemData(VM::t_addr addr)
{
	// get data type info from memory
	t_byte tyval = ReadMemRaw<t_byte>(addr);
	addr += m_bytesize;
	VMType ty = static_cast<VMType>(tyval);

	t_data dat;
	switch(ty)
	{
		case VMType::REAL:
		{
			t_real val = ReadMemRaw<t_real>(addr);
			std::get<t_real>(dat) = val;

			//std::cout << "read real " << val << " from address " << (addr-1) << std::endl;
			break;
		}

		case VMType::INT:
		{
			t_int val = ReadMemRaw<t_int>(addr);
			dat = val;
			break;
		}

		case VMType::ADDR_MEM:
		case VMType::ADDR_IP:
		case VMType::ADDR_SP:
		case VMType::ADDR_BP:
		case VMType::ADDR_GBP:
		{
			t_addr val = ReadMemRaw<t_addr>(addr);
			dat = val;
			break;
		}

		default:
		{
			throw std::runtime_error("ReadMem: Data type not yet implemented");
			break;
		}
	}

	return std::make_tuple(ty, dat);
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
	/*else if(std::holds_alternative<t_addr>(data))
	{
		//std::cout << "writing address value " << std::get<T_ADDR>(data) << " to address " << addr << std::endl;

		// write descriptor prefix
		WriteMemRaw<t_byte>(addr, static_cast<t_byte>(ty));
		addr += m_bytesize;

		// write the actual data
		WriteMemRaw<t_int>(addr, std::get<T_ADDR>(data));
	}*/
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
	else if(std::holds_alternative<t_addr>(data))
		return m_addrsize;

	return 0;
}


void VM::Reset()
{
	m_ip = 0;
	m_sp = m_memsize - m_framesize;
	m_bp = m_memsize;
	m_bp -= m_realsize + 1; // padding of max. data type size to avoid writing beyond memory size
	m_gbp = m_bp;

	std::memset(m_mem.get(), 0, m_memsize);
}


void VM::SetMem(t_addr addr, t_byte data)
{
	CheckMemoryBounds(addr, sizeof(t_byte));

	m_mem[addr % m_memsize] = data;
}


void VM::SetMem(t_addr addr, const std::string& data)
{
	for(std::size_t i=0; i<data.size(); ++i)
		SetMem(addr + (t_addr)(i), static_cast<t_byte>(data[i]));
}
