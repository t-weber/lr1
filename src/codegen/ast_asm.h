/**
 * ast asm generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_AST_ASM_H__
#define __LR1_AST_ASM_H__

#include <unordered_map>
#include <tuple>
#include <iostream>
#include <cstdint>

#include "lval.h"
#include "ast.h"
#include "../vm/opcodes.h"


class ASTAsm : public ASTVisitor
{
public:
	ASTAsm(std::ostream& ostr = std::cout,
		std::unordered_map<std::size_t, std::tuple<std::string, OpCode>> *ops = nullptr);

	ASTAsm(const ASTAsm&) = delete;
	const ASTAsm& operator=(const ASTAsm&) = delete;

	virtual void visit(const ASTToken<t_lval>* ast, std::size_t level) const override;
	virtual void visit(const ASTToken<t_real>* ast, std::size_t level) const override;
	virtual void visit(const ASTToken<t_int>* ast, std::size_t level) const override;
	virtual void visit(const ASTToken<std::string>* ast, std::size_t level) const override;
	virtual void visit(const ASTToken<void*>* ast, std::size_t level) const override;
	virtual void visit(const ASTDelegate* ast, std::size_t level) const override;
	virtual void visit(const ASTUnary* ast, std::size_t level) const override;
	virtual void visit(const ASTBinary* ast, std::size_t level) const override;

	void SetStream(std::ostream* ostr) { m_ostr = ostr; }
	void SetBinary(bool bin) { m_binary = bin; }


private:
	std::ostream* m_ostr{&std::cout};
	const std::unordered_map<std::size_t, std::tuple<std::string, OpCode>> *m_ops{nullptr};
	bool m_binary{false};
};


#endif
