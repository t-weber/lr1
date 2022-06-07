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
		//m_ostr->put(std::to_underlying(OpCode::PUSH));
		m_ostr->put(static_cast<std::int8_t>(OpCode::PUSH));
		m_ostr->write(reinterpret_cast<const char*>(&val), sizeof(t_real));
	}
	else
	{
		(*m_ostr) << "push " << val << std::endl;
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
		(*m_ostr) << "push " << val << std::endl;
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
		if(op == OpCode::ADD)
			op = OpCode::UADD;
		if(op == OpCode::SUB)
			op = OpCode::USUB;
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
