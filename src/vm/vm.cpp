/**
 * zero-address code vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 8-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "vm.h"

#include <iostream>
#include <sstream>
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

		if(m_debug)
		{
			std::cout << "*** read instruction at ip = " << t_int(m_ip)
				<< ", opcode: " << std::hex
				<< static_cast<std::size_t>(op)
				<< " (" << get_vm_opcode_name(op) << ")"
				<< std::dec << ". ***" << std::endl;
		}

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

				if(m_debug)
				{
					std::cout << "dereferenced address "
						<< addr << ": "
						//<< std::get<t_real>(val)
						<< "." << std::endl;
				}
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

			case OpCode::BINAND:
			{
				OpBinary<'&'>();
				break;
			}

			case OpCode::BINOR:
			{
				OpBinary<'|'>();
				break;
			}

			case OpCode::BINXOR:
			{
				OpBinary<'^'>();
				break;
			}

			case OpCode::BINNOT:
			{
				t_data val = PopData();
				if(std::holds_alternative<t_int>(val))
				{
					t_int newval = ~std::get<t_int>(val);
					PushData(newval);
				}
				else
				{
					throw std::runtime_error("Invalid data type for binary not.");
				}

				break;
			}

			case OpCode::SHL:
			{
				OpBinary<'<'>();
				break;
			}

			case OpCode::SHR:
			{
				OpBinary<'>'>();
				break;
			}

			case OpCode::ROTL:
			{
				OpBinary<'l'>();
				break;
			}

			case OpCode::ROTR:
			{
				OpBinary<'r'>();
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
				OpCast<t_int>();
				break;
			}

			case OpCode::TOF: // converts value to t_real
			{
				OpCast<t_real>();
				break;
			}

			case OpCode::TOS: // converts value to t_str
			{
				OpCast<t_str>();
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

			/**
			 * stack frame for functions:
			 *
			 *  --------------------
			 * |  local var n       |  <-- m_sp
			 *  --------------------      |
			 * |      ...           |     |
			 *  --------------------      |
			 * |  local var 2       |     |  m_framesize
			 *  --------------------      |
			 * |  local var 1       |     |
			 *  --------------------      |
			 * |  old m_bp          |  <-- m_bp (= previous m_sp)
			 *  --------------------
			 * |  old m_ip for ret  |
			 *  --------------------
			 * |  func. arg 1       |
			 *  --------------------
			 * |  func. arg 2       |
			 *  --------------------
			 * |  ...               |
			 *  --------------------
			 * |  func. arg n       |
			 *  --------------------
			 */
			case OpCode::CALL: // function call
			{
				t_addr funcaddr = PopAddress();

				// save instruction and base pointer and
				// set up the function's stack frame for local variables
				PushAddress(m_ip, VMType::ADDR_MEM);
				PushAddress(m_bp, VMType::ADDR_MEM);

				if(m_debug)
				{
					std::cout << "saved base pointer "
						<< m_bp << "."
						<< std::endl;
				}
				m_bp = m_sp;
				m_sp -= m_framesize;

				// jump to function
				m_ip = funcaddr;
				if(m_debug)
				{
					std::cout << "calling function "
						<< funcaddr << "."
						<< std::endl;
				}
				break;
			}

			case OpCode::RET: // return from function
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

				if(m_debug)
				{
					std::cout << "restored base pointer "
						<< m_bp << "."
						<< std::endl;
				}

				// remove function arguments from stack
				for(t_int arg=0; arg<num_args; ++arg)
					PopData();

				PushData(retval, VMType::UNKNOWN, false);
				break;
			}

			case OpCode::EXTCALL: // external function call
			{
				// get function name
				std::string funcname = std::get<t_str>(PopData());

				// get number of function arguments
				t_int num_args = std::get<t_int>(PopData());

				if(m_debug)
				{
					std::cout << "external function call to \"" << funcname
						<< "\" with " << num_args << " arguments."
						<< "." << std::endl;
				}

				// TODO

				//PushData(retval, VMType::UNKNOWN, false);
				break;
			}

			default:
			{
				std::cerr << "Error: Invalid instruction " << std::hex
					<< static_cast<t_addr>(_op) << std::dec
					<< std::endl;
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
	VMType thereg = static_cast<VMType>(regval);

	if(m_debug)
	{
		std::cout << "popped address " << t_int(addr)
			<< " of type " << t_int(regval)
			<< " (" << get_vm_type_name(thereg) << ")"
			<< "." << std::endl;
	}

	// get absolute address using base address from register
	switch(thereg)
	{
		case VMType::ADDR_MEM: break;
		case VMType::ADDR_IP: addr += m_ip; break;
		case VMType::ADDR_SP: addr += m_sp; break;
		case VMType::ADDR_BP: addr += m_bp; break;
		case VMType::ADDR_GBP: addr += m_gbp; break;
		//case VMType::ADDR_BP_ARG: break;
		default: throw std::runtime_error("Unknown address base register."); break;
	}

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
 * pop a string from the stack
 * a string consists of an t_addr giving the length
 * following by the string (without 0-termination)
 */
VM::t_str VM::PopString()
{
	t_addr len = PopRaw<t_addr, m_addrsize>();
	CheckMemoryBounds(m_sp, len);

	t_char* begin = reinterpret_cast<t_char*>(m_mem.get() + m_sp);
	t_str str(begin, len);
	m_sp += len;

	return str;
}


/**
 * get a string from the top of the stack
 */
VM::t_str VM::TopString(t_addr sp_offs) const
{
	t_addr len = TopRaw<t_addr, m_addrsize>(sp_offs);
	t_addr addr = m_sp + sp_offs + m_addrsize;

	CheckMemoryBounds(addr, len);
	t_char* begin = reinterpret_cast<t_char*>(m_mem.get() + addr);
	t_str str(begin, len);

	return str;
}


/**
 * push a string to the stack
 */
void VM::PushString(const VM::t_str& str)
{
	t_addr len = static_cast<t_addr>(str.length());
	CheckMemoryBounds(m_sp, len);

	m_sp -= len;
	t_char* begin = reinterpret_cast<t_char*>(m_mem.get() + m_sp);
	std::memcpy(begin, str.data(), len);

	PushRaw<t_addr, m_addrsize>(len);
}


/**
 * get top data from the stack, which is prefixed
 * with a type descriptor byte
 */
VM::t_data VM::TopData() const
{
	// get data type info from stack
	t_byte tyval = TopRaw<t_byte, m_bytesize>();
	VMType ty = static_cast<VMType>(tyval);

	t_data dat;

	switch(ty)
	{
		case VMType::REAL:
		{
			dat = TopRaw<t_real, m_realsize>(m_bytesize);
			break;
		}

		case VMType::INT:
		{
			dat = TopRaw<t_int, m_intsize>(m_bytesize);
			break;
		}

		case VMType::ADDR_MEM:
		case VMType::ADDR_IP:
		case VMType::ADDR_SP:
		case VMType::ADDR_BP:
		case VMType::ADDR_GBP:
		{
			dat = TopRaw<t_addr, m_addrsize>(m_bytesize);
			break;
		}

		case VMType::STR:
		{
			dat = TopString(m_bytesize);
			break;
		}

		default:
		{
			std::ostringstream msg;
			msg << "Top: Data type " << (int)tyval
				<< " (" << get_vm_type_name(ty) << ")"
				<< " not yet implemented.";
			throw std::runtime_error(msg.str());
			break;
		}
	}

	return dat;
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
			dat = PopRaw<t_real, m_realsize>();
			if(m_debug)
			{
				std::cout << "popped real " << std::get<t_real>(dat)
					<< "." << std::endl;
			}
			break;
		}

		case VMType::INT:
		{
			dat = PopRaw<t_int, m_intsize>();
			if(m_debug)
			{
				std::cout << "popped int " << std::get<t_int>(dat)
					<< "." << std::endl;
			}
			break;
		}

		case VMType::ADDR_MEM:
		case VMType::ADDR_IP:
		case VMType::ADDR_SP:
		case VMType::ADDR_BP:
		case VMType::ADDR_GBP:
		{
			dat = PopRaw<t_addr, m_addrsize>();
			if(m_debug)
			{
				std::cout << "popped address " << std::get<t_addr>(dat)
					<< "." << std::endl;
			}
			break;
		}

		case VMType::STR:
		{
			dat = PopString();
			if(m_debug)
			{
				std::cout << "popped string \"" << std::get<t_str>(dat)
					<< "\"." << std::endl;
			}
			break;
		}

		default:
		{
			std::ostringstream msg;
			msg << "Pop: Data type " << (int)tyval
				<< " (" << get_vm_type_name(ty) << ")"
				<< " not yet implemented.";
			throw std::runtime_error(msg.str());
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
		if(m_debug)
		{
			std::cout << "pushing real "
				<< std::get<t_real>(data) << "."
				<< std::endl;
		}

		// push the actual data
		PushRaw<t_real, m_realsize>(std::get<t_real>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(VMType::REAL));
	}
	else if(std::holds_alternative<t_int>(data))
	{
		if(m_debug)
		{
			std::cout << "pushing int "
				<< std::get<t_int>(data) << "."
				<< std::endl;
		}

		// push the actual data
		PushRaw<t_int, m_intsize>(std::get<t_int>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(VMType::INT));
	}
	else if(std::holds_alternative<t_addr>(data))
	{
		if(m_debug)
		{
			std::cout << "pushing address "
				<< std::get<t_addr>(data) << "."
				<< std::endl;
		}

		// push the actual address
		PushRaw<t_addr, m_addrsize>(std::get<t_addr>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(ty));
	}
	else if(std::holds_alternative<t_str>(data))
	{
		if(m_debug)
		{
			std::cout << "pushing string \""
				<< std::get<t_str>(data) << "\"."
				<< std::endl;
		}

		// push the actual string
		PushString(std::get<t_str>(data));

		// push descriptor
		PushRaw<t_byte, m_bytesize>(static_cast<t_byte>(VMType::STR));
	}
	else if(err_on_unknown)
	{
		std::ostringstream msg;
		msg << "Push: Data type " << (int)ty
			<< " (" << get_vm_type_name(ty) << ")"
			<< " not yet implemented.";
		throw std::runtime_error(msg.str());
	}
}


/**
 * get the address of a function argument variable
 */
VM::t_addr VM::GetArgAddr(VM::t_addr addr, VM::t_addr arg_num) const
{
	// skip to correct argument index
	while(arg_num > 0)
	{
		// get data type info from memory
		VMType ty = static_cast<VMType>(ReadMemRaw<t_byte>(addr));
		addr += m_bytesize;

		switch(ty)
		{
			case VMType::REAL:
				addr += m_realsize;
				break;
			case VMType::INT:
				addr += m_intsize;
				break;
			case VMType::ADDR_MEM:
			case VMType::ADDR_IP:
			case VMType::ADDR_SP:
			case VMType::ADDR_BP:
			case VMType::ADDR_GBP:
				addr += m_addrsize;
				break;
			case VMType::STR:
			{
				t_addr len = ReadMemRaw<t_addr>(addr);
				addr += m_addrsize + len;
				break;
			}
			default:
			{
				std::ostringstream msg;
				msg << "GetArgAddr: Data type " << (int)ty
					<< " (" << get_vm_type_name(ty) << ")"
					<< " not yet implemented.";
				throw std::runtime_error(msg.str());
			}
		}

		--arg_num;
	}

	return addr;
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
			dat = val;

			if(m_debug)
			{
				std::cout << "read real " << val
					<< " from address " << (addr-1)
					<< "." << std::endl;
			}
			break;
		}

		case VMType::INT:
		{
			t_int val = ReadMemRaw<t_int>(addr);
			dat = val;

			if(m_debug)
			{
				std::cout << "read int " << val
					<< " from address " << (addr-1)
					<< "." << std::endl;
			}
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

			if(m_debug)
			{
				std::cout << "read address " << t_int(val)
					<< " from address " << t_int(addr-1)
					<< "." << std::endl;
			}
			break;
		}

		case VMType::ADDR_BP_ARG:
		{
			t_addr arg_num = ReadMemRaw<t_addr>(addr);
			t_addr arg_addr = GetArgAddr(m_bp, arg_num) - m_bp;
			ty = VMType::ADDR_BP;

			dat = arg_addr;

			if(m_debug)
			{
				std::cout << "read address " << t_int(arg_addr)
					<< " for argument #" << t_int(arg_num)
					<< "." << std::endl;
			}

			// dereference:
			//std::tie(ty, dat, std::ignore) = ReadMemData(arg_addr);
			break;
		}

		case VMType::STR:
		{
			t_str str = ReadMemRaw<t_str>(addr);
			dat = str;

			if(m_debug)
			{
				std::cout << "read string \"" << str
					<< "\" from address " << (addr-1)
					<< "." << std::endl;
			}
			break;
		}

		default:
		{
			std::ostringstream msg;
			msg << "ReadMem: Data type " << (int)tyval
				<< " (" << get_vm_type_name(ty) << ")"
				<< " not yet implemented.";
			throw std::runtime_error(msg.str());
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
		if(m_debug)
		{
			std::cout << "writing real value "
				<< std::get<t_real>(data)
				<< " to address " << addr
				<< "." << std::endl;
		}

		// write descriptor prefix
		WriteMemRaw<t_byte>(addr, static_cast<t_byte>(VMType::REAL));
		addr += m_bytesize;

		// write the actual data
		WriteMemRaw<t_real>(addr, std::get<t_real>(data));
	}
	else if(std::holds_alternative<t_int>(data))
	{
		if(m_debug)
		{
			std::cout << "writing int value "
				<< std::get<t_int>(data)
				<< " to address " << addr
				<< "." << std::endl;
		}

		// write descriptor prefix
		WriteMemRaw<t_byte>(addr, static_cast<t_byte>(VMType::INT));
		addr += m_bytesize;

		// write the actual data
		WriteMemRaw<t_int>(addr, std::get<t_int>(data));
	}
	/*else if(std::holds_alternative<t_addr>(data))
	{
		if(m_debug)
		{
			std::cout << "writing address value "
				<< std::get<t_addr>(data)
				<< " to address " << addr
				<< std::endl;
		}

		// write descriptor prefix
		WriteMemRaw<t_byte>(addr, static_cast<t_byte>(ty));
		addr += m_bytesize;

		// write the actual data
		WriteMemRaw<t_int>(addr, std::get<t_addr>(data));
	}*/
	else if(std::holds_alternative<t_str>(data))
	{
		if(m_debug)
		{
			std::cout << "writing string \""
				<< std::get<t_str>(data)
				<< "\" to address " << addr
				<< "." << std::endl;
		}

		// write descriptor prefix
		WriteMemRaw<t_byte>(addr, static_cast<t_byte>(VMType::STR));
		addr += m_bytesize;

		// write the actual data
		WriteMemRaw<t_str>(addr, std::get<t_str>(data));
	}
	else
	{
		throw std::runtime_error("WriteMem: Data type not yet implemented.");
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
	else if(std::holds_alternative<t_str>(data))
		return m_addrsize /*len*/ + std::get<t_str>(data).length();

	throw std::runtime_error("GetDataSize: Data type not yet implemented.");
	return 0;
}


void VM::Reset()
{
	m_ip = 0;
	m_sp = m_memsize - m_framesize;
	m_bp = m_memsize;
	m_bp -= sizeof(t_data) + 1; // padding of max. data type size to avoid writing beyond memory size
	m_gbp = m_bp;

	std::memset(m_mem.get(), 0, m_memsize);
}


void VM::SetMem(t_addr addr, VM::t_byte data)
{
	CheckMemoryBounds(addr, sizeof(t_byte));

	m_mem[addr % m_memsize] = data;
}


void VM::SetMem(t_addr addr, const std::string& data)
{
	for(std::size_t i=0; i<data.size(); ++i)
		SetMem(addr + (t_addr)(i), static_cast<t_byte>(data[i]));
}


void VM::SetMem(t_addr addr, const VM::t_byte* data, std::size_t size)
{
	for(std::size_t i=0; i<size; ++i)
		SetMem(addr + t_addr(i), data[i]);
}
