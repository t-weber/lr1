/**
 * zero-address asm code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#include "ast_asm.h"


ASTAsm::ASTAsm(std::ostream& ostr,
	std::unordered_map<std::size_t, std::tuple<std::string, OpCode>> *ops)
	: m_ostr{&ostr}, m_ops{ops}
{
}


void ASTAsm::visit(
	[[maybe_unused]] const ASTToken<t_lval>* ast,
	[[maybe_unused]] std::size_t level)
{
	std::cerr << "Error: " << __func__ << " not implemented." << std::endl;
}


void ASTAsm::visit(const ASTToken<t_real>* ast,
	[[maybe_unused]] std::size_t level)
{
	if(!ast->HasLexerValue())
		return;
	t_real val = ast->GetLexerValue();

	if(m_binary)
	{
		//m_ostr->put(std::to_underlying(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		// write type descriptor byte
		m_ostr->put(static_cast<t_vm_byte>(VMType::REAL));
		// write value
		m_ostr->write(reinterpret_cast<const char*>(&val), sizeof(t_real));
	}
	else
	{
		(*m_ostr) << "push " << val << std::endl;
	}
}


void ASTAsm::visit(const ASTToken<t_int>* ast,
	[[maybe_unused]] std::size_t level)
{
	if(!ast->HasLexerValue())
		return;
	t_int val = ast->GetLexerValue();

	if(m_binary)
	{
		//m_ostr->put(std::to_underlying(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		// write type descriptor byte
		m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
		// write data
		m_ostr->write(reinterpret_cast<const char*>(&val), sizeof(t_int));
	}
	else
	{
		(*m_ostr) << "push " << val << std::endl;
	}
}


void ASTAsm::visit(const ASTToken<std::string>* ast,
	[[maybe_unused]] std::size_t level)
{
	if(!ast->HasLexerValue())
		return;

	const std::string& val = ast->GetLexerValue();
	bool lval = ast->IsLValue();

	if(m_binary)
	{
		std::string varname;
		if(m_cur_func != "")
			varname = m_cur_func + "/" + val;
		else
			varname = val;

		// get variable address and push it
		const SymInfo *sym = m_symtab.GetSymbol(varname);
		if(!sym)
		{
			if(m_cur_func == "")
			{
				// in global scope
				sym = m_symtab.AddSymbol(varname, -m_glob_stack, VMType::ADDR_GBP);
				m_glob_stack += sizeof(t_real) + 1;  // data and descriptor size
			}
			else
			{
				// in local function scope
				if(m_local_stack.find(m_cur_func) == m_local_stack.end())
				{
					// padding of maximum data type size, to avoid overwriting stack value
					m_local_stack[m_cur_func] = sizeof(t_real) + 1;
				}

				sym = m_symtab.AddSymbol(varname, -m_local_stack[m_cur_func], VMType::ADDR_BP);
				m_local_stack[m_cur_func] += sizeof(t_real) + 1;  // data and descriptor size
			}
		}

		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		// register with base address
		m_ostr->put(static_cast<t_vm_byte>(sym->loc));
		// relative address
		m_ostr->write(reinterpret_cast<const char*>(&sym->addr), sizeof(t_vm_addr));

		// dereference it, if the variable is on the rhs of an assignment
		if(!lval)
			m_ostr->put(static_cast<t_vm_byte>(OpCode::DEREF));
	}
	else
	{
		(*m_ostr) << "push addr " << val << std::endl;

		// dereference it, if the variable is on the rhs of an assignment
		if(!lval)
			(*m_ostr) << "deref" << std::endl;
	}
}


void ASTAsm::visit(
	[[maybe_unused]] const ASTToken<void*>* ast,
	[[maybe_unused]] std::size_t level)
{
	std::cerr << "Error: " << __func__ << " not implemented." << std::endl;
}


void ASTAsm::visit(
	[[maybe_unused]] const ASTDelegate* ast,
	[[maybe_unused]] std::size_t level)
{
	for(std::size_t i=0; i<ast->NumChildren(); ++i)
		ast->GetChild(i)->accept(this, level+1);
}


void ASTAsm::visit(const ASTUnary* ast, [[maybe_unused]] std::size_t level)
{
	ast->GetChild(0)->accept(this, level+1);

	std::size_t opid = ast->GetOpId();

	if(m_binary)
	{
		OpCode op = std::get<OpCode>(m_ops->at(opid));
		if(op == OpCode::ADD) op = OpCode::NOP;
		else if(op == OpCode::SUB) op = OpCode::USUB;
		m_ostr->put(static_cast<t_vm_byte>(op));
	}
	else
	{
		if(m_ops)
			(*m_ostr) << "u" << std::get<std::string>(m_ops->at(opid));
		else
			(*m_ostr) << static_cast<char>(opid);
		(*m_ostr) << std::endl;
	}
}


void ASTAsm::visit(const ASTBinary* ast, [[maybe_unused]] std::size_t level)
{
	ast->GetChild(0)->accept(this, level+1); // operand 1
	ast->GetChild(1)->accept(this, level+1); // operand 2

	std::size_t opid = ast->GetOpId();

	if(m_binary)
	{
		OpCode op = std::get<OpCode>(m_ops->at(opid));
		if(op != OpCode::INVALID)	// use opcode directly
		{
			m_ostr->put(static_cast<t_vm_byte>(op));
		}
		else	// decide on special cases
		{
			// TODO
		}
	}
	else
	{
		if(m_ops)
			(*m_ostr) << std::get<std::string>(m_ops->at(opid));
		else
			(*m_ostr) << static_cast<char>(opid);
		(*m_ostr) << std::endl;
	}
}


void ASTAsm::visit(const ASTList* ast, [[maybe_unused]] std::size_t level)
{
	for(std::size_t i=0; i<ast->NumChildren(); ++i)
		ast->GetChild(i)->accept(this, level+1);
}


void ASTAsm::visit(const ASTCondition* ast, [[maybe_unused]] std::size_t level)
{
	// condition
	ast->GetCondition()->accept(this, level+1);

	std::size_t labelEndCond = m_glob_label++;
	std::size_t labelEndIf = m_glob_label++;

	t_vm_addr skipEndCond = 0;         // how many bytes to skip to jump to end of the if block?
	t_vm_addr skipEndIf = 0;           // how many bytes to skip to jump to end of the entire if statement?
	std::streampos skip_addr = 0;      // stream position with the condition jump label
	std::streampos skip_else_addr = 0; // stream position with the if block jump label

	if(m_binary)
	{
		// if the condition is not fulfilled...
		m_ostr->put(static_cast<t_vm_byte>(OpCode::NOT));

		// ...skip to the end of the if block
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
		skip_addr = m_ostr->tellp();
		m_ostr->write(reinterpret_cast<const char*>(&skipEndCond), sizeof(t_vm_addr));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::JMPCND));
	}
	else
	{
		(*m_ostr) << "not\n";
		(*m_ostr) << "push addr end_cond_" << labelEndCond << "\n";
		(*m_ostr) << "jmpcnd\n";
	}

	// if block
	std::streampos before_if_block = m_ostr->tellp();
	ast->GetIfBlock()->accept(this, level+1);
	if(ast->GetElseBlock())
	{
		if(m_binary)
		{
			// skip to end of if statement if there's an else block
			m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
			m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
			skip_else_addr = m_ostr->tellp();
			m_ostr->write(reinterpret_cast<const char*>(&skipEndIf), sizeof(t_vm_addr));
			m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));
		}
		else
		{
			(*m_ostr) << "push addr end_if_" << labelEndIf << "\n";
			(*m_ostr) << "jmp\n";
		}
	}
	std::streampos after_if_block = m_ostr->tellp();

	if(m_binary)
	{
		// go back and fill in missing number of bytes to skip
		skipEndCond = after_if_block - before_if_block;
		m_ostr->seekp(skip_addr);
		m_ostr->write(reinterpret_cast<const char*>(&skipEndCond), sizeof(t_vm_addr));
		m_ostr->seekp(after_if_block);
	}
	else
	{
		(*m_ostr) << "end_cond_" << labelEndCond << ":" << std::endl;
	}

	// else block
	if(ast->GetElseBlock())
	{
		std::streampos before_else_block = m_ostr->tellp();
		ast->GetElseBlock()->accept(this, level+1);
		std::streampos after_else_block = m_ostr->tellp();

		if(m_binary)
		{
			// go back and fill in missing number of bytes to skip
			skipEndIf = after_else_block - before_else_block;
			m_ostr->seekp(skip_else_addr);
			m_ostr->write(reinterpret_cast<const char*>(&skipEndIf), sizeof(t_vm_addr));
			m_ostr->seekp(after_else_block);
		}
		else
		{
			(*m_ostr) << "end_if_" << labelEndIf << ":" << std::endl;
		}
	}
}


void ASTAsm::visit(const ASTLoop* ast, [[maybe_unused]] std::size_t level)
{
	std::size_t labelBegin = m_glob_label++;
	std::size_t labelEnd = m_glob_label++;
	std::streampos loop_begin = m_ostr->tellp();

	if(!m_binary)
	{
		(*m_ostr) << "begin_loop_" << labelBegin << ":" << std::endl;
	}

	ast->GetCondition()->accept(this, level+1); // condition

	t_vm_addr skip = 0;  // how many bytes to skip to jump to end of the block?
	std::streampos skip_addr = 0;

	if(m_binary)
	{
		m_ostr->put(static_cast<t_vm_byte>(OpCode::NOT));

		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
		skip_addr = m_ostr->tellp();
		m_ostr->write(reinterpret_cast<const char*>(&skip), sizeof(t_vm_addr));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::JMPCND));
	}
	else
	{
		(*m_ostr) << "not\n";
		(*m_ostr) << "push addr end_loop_" << labelEnd << "\n";
		(*m_ostr) << "jmpcnd\n";
	}

	std::streampos before_block = m_ostr->tellp();
	ast->GetBlock()->accept(this, level+1); // block

	if(m_binary)
	{
		// loop back
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
		std::streampos after_block = m_ostr->tellp();
		skip = after_block - before_block;
		t_vm_addr skip_back = loop_begin - after_block;
		skip_back -= static_cast<t_vm_addr>(sizeof(t_vm_addr)); // include the next two writes
		skip_back -= static_cast<t_vm_addr>(sizeof(t_vm_byte));
		m_ostr->write(reinterpret_cast<const char*>(&skip_back), sizeof(t_vm_addr));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));

		// go back and fill in missing number of bytes to skip
		after_block = m_ostr->tellp();
		skip = after_block - before_block;
		m_ostr->seekp(skip_addr);
		m_ostr->write(reinterpret_cast<const char*>(&skip), sizeof(t_vm_addr));
		m_ostr->seekp(after_block);
	}
	else
	{
		(*m_ostr) << "push addr begin_loop_" << labelBegin << "\n";
		(*m_ostr) << "jmp\n";

		(*m_ostr) << "end_loop_" << labelEnd << ":" << std::endl;
	}
}


void ASTAsm::visit(const ASTFunc* ast, [[maybe_unused]] std::size_t level)
{
	if(m_cur_func != "")
		throw std::runtime_error("Nested functions are not allowed.");

	// function name
	m_cur_func = ast->GetName();

	// function arguments
	t_int num_args = 0;
	if(ast->GetArgs())
	{
		num_args = ast->GetArgs()->NumChildren();
	}

	std::streampos jmp_end_streampos;
	t_vm_addr end_func_addr = 0;

	if(m_binary)
	{
		// jump to the end of the function to prevent accidental execution
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
		jmp_end_streampos = m_ostr->tellp();
		m_ostr->write(reinterpret_cast<const char*>(&end_func_addr), sizeof(t_vm_addr));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));
	}
	else
	{
		(*m_ostr) << "jmp " << ast->GetName() << "_end\n";
		(*m_ostr) << ast->GetName() << ":\n";
	}


	// function arguments
	if(m_binary && ast->GetArgs())
	{
		// skip saved ebp and return address on stack frame
		t_vm_addr addr_bp = 2 * (sizeof(t_vm_addr) + 1);

		for(std::size_t i=0; i<ast->GetArgs()->NumChildren(); ++i)
		{
			auto ident = std::dynamic_pointer_cast<ASTToken<std::string>>(ast->GetArgs()->GetChild(i));
			const std::string& argname = ident->GetLexerValue();
			std::string varname = m_cur_func + "/" + argname;

			//std::cout << "arg: " << varname << ", addr: " << addr_bp << std::endl;
			m_symtab.AddSymbol(varname, addr_bp, VMType::ADDR_BP);
			addr_bp += sizeof(t_real) + 1;  // TODO: implement for other data types
		}
	}


	std::streampos before_block = m_ostr->tellp();

	if(m_binary)
	{
		// add function to symbol table
		m_symtab.AddSymbol(ast->GetName(), before_block, VMType::ADDR_MEM, true, 0);
		//std::cout << "function " << ast->GetName() << " at address " << before_block << std::endl;
	}

	ast->GetBlock()->accept(this, level+1); // block


	if(m_binary)
	{
		// push number of arguments
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
		m_ostr->write(reinterpret_cast<const char*>(&num_args), sizeof(t_int));

		m_ostr->put(static_cast<t_vm_byte>(OpCode::RET));

		// fill in end-of-function jump address
		std::streampos end_func_streampos = m_ostr->tellp();
		end_func_addr = end_func_streampos - before_block;
		m_ostr->seekp(jmp_end_streampos);
		m_ostr->write(reinterpret_cast<const char*>(&end_func_addr), sizeof(t_vm_addr));
		m_ostr->seekp(end_func_streampos);
	}
	else
	{
		(*m_ostr) << "ret " << num_args << "\n";
		(*m_ostr) << ast->GetName() << "_end:" << std::endl;
	}

	m_cur_func = "";
}


void ASTAsm::visit(const ASTFuncCall* ast, [[maybe_unused]] std::size_t level)
{
	//std::size_t num_args = ast->GetArgs()->NumChildren();

	// push the function arguments
	if(ast->GetArgs())
		ast->GetArgs()->accept(this, level+1);

	if(m_binary)
	{
		// get function address and push it
		const SymInfo *sym = m_symtab.GetSymbol(ast->GetName());
		if(!sym)
		{
			throw std::runtime_error("Tried to call unknown function \"" + ast->GetName() + "\".");
		}

		// push function address
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
		m_ostr->write(reinterpret_cast<const char*>(&sym->addr), sizeof(t_vm_addr));

		m_ostr->put(static_cast<t_vm_byte>(OpCode::CALL));
	}
	else
	{
		(*m_ostr) << "call " << ast->GetName() << std::endl;
	}
}
