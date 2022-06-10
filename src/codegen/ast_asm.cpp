/**
 * zero-address asm code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#include "ast_asm.h"
//#include <utility>


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
		//m_ostr->put(std::to_underlying(OpCode::PUSHF));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSHF));
		m_ostr->write(reinterpret_cast<const char*>(&val), sizeof(t_real));
	}
	else
	{
		(*m_ostr) << "pushf " << val << std::endl;
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
		//m_ostr->put(std::to_underlying(OpCode::PUSHI));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSHI));
		m_ostr->write(reinterpret_cast<const char*>(&val), sizeof(t_int));
	}
	else
	{
		(*m_ostr) << "pushi " << val << std::endl;
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
		// get variable address and push it
		const SymInfo *sym = m_symtab.GetSymbol(val);
		if(!sym)
		{
			sym = m_symtab.AddSymbol(val, m_glob_stack, Register::BP);
			m_glob_stack += sizeof(t_real);
		}

		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSHADDR));
		// register with base address
		m_ostr->put(static_cast<t_vm_byte>(sym->loc));
		// relative address
		m_ostr->write(reinterpret_cast<const char*>(&sym->addr), sizeof(t_vm_addr));

		// dereference it, if the variable is on the rhs of an assignment
		if(!lval)
			m_ostr->put(static_cast<t_vm_byte>(OpCode::DEREFF));
	}
	else
	{
		(*m_ostr) << "pushaddr " << val << std::endl;

		// dereference it, if the variable is on the rhs of an assignment
		if(!lval)
			(*m_ostr) << "dereff" << std::endl;
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
		if(op == OpCode::ADDF) op = OpCode::NOP;
		else if(op == OpCode::SUBF) op = OpCode::USUBF;
		else if(op == OpCode::ADDI) op = OpCode::NOP;
		else if(op == OpCode::SUBI) op = OpCode::USUBI;
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
