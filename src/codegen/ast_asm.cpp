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
	[[maybe_unused]] std::size_t level) const
{
	std::cerr << "Error: " << __func__ << " not implemented." << std::endl;
}


void ASTAsm::visit(const ASTToken<t_real>* ast,
	[[maybe_unused]] std::size_t level) const
{
	if(!ast->HasLexerValue())
		return;
	t_real val = ast->GetLexerValue();

	if(m_binary)
	{
		//m_ostr->put(std::to_underlying(OpCode::PUSHF));
		m_ostr->put(static_cast<std::int8_t>(OpCode::PUSHF));
		m_ostr->write(reinterpret_cast<const char*>(&val), sizeof(t_real));
	}
	else
	{
		(*m_ostr) << "pushf " << val << std::endl;
	}
}


void ASTAsm::visit(const ASTToken<t_int>* ast,
	[[maybe_unused]] std::size_t level) const
{
	if(!ast->HasLexerValue())
		return;
	t_int val = ast->GetLexerValue();

	if(m_binary)
	{
		//m_ostr->put(std::to_underlying(OpCode::PUSHI));
		m_ostr->put(static_cast<std::int8_t>(OpCode::PUSHI));
		m_ostr->write(reinterpret_cast<const char*>(&val), sizeof(t_int));
	}
	else
	{
		(*m_ostr) << "pushi " << val << std::endl;
	}
}


void ASTAsm::visit(const ASTToken<std::string>* ast,
	[[maybe_unused]] std::size_t level) const
{
	if(!ast->HasLexerValue())
		return;
	const std::string& val = ast->GetLexerValue();

	if(m_binary)
	{
		// TODO
	}
	else
	{
		// TODO
		(*m_ostr) << "pushstr " << val << std::endl;
	}
}


void ASTAsm::visit(
	[[maybe_unused]] const ASTToken<void*>* ast,
	[[maybe_unused]] std::size_t level) const
{
	std::cerr << "Error: " << __func__ << " not implemented." << std::endl;
}


void ASTAsm::visit(
	[[maybe_unused]] const ASTDelegate* ast,
	[[maybe_unused]] std::size_t level) const
{
	for(std::size_t i=0; i<ast->NumChildren(); ++i)
		ast->GetChild(i)->accept(this, level+1);
}


void ASTAsm::visit(const ASTUnary* ast, [[maybe_unused]] std::size_t level) const
{
	ast->GetChild(0)->accept(this, level+1);

	std::size_t opid = ast->GetOpId();

	if(m_binary)
	{
		OpCode op = std::get<OpCode>(m_ops->at(opid));
		if(op == OpCode::ADDF) op = OpCode::UADDF;
		else if(op == OpCode::SUBF) op = OpCode::USUBF;
		else if(op == OpCode::ADDI) op = OpCode::UADDI;
		else if(op == OpCode::SUBI) op = OpCode::USUBI;
		m_ostr->put(static_cast<std::int8_t>(op));
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


void ASTAsm::visit(const ASTBinary* ast, [[maybe_unused]] std::size_t level) const
{
	ast->GetChild(0)->accept(this, level+1);
	ast->GetChild(1)->accept(this, level+1);

	std::size_t opid = ast->GetOpId();

	if(m_binary)
	{
		OpCode op = std::get<OpCode>(m_ops->at(opid));
		m_ostr->put(static_cast<std::int8_t>(op));
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
