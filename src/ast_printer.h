/**
 * ast printer
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_AST_PRINTER_H__
#define __LR1_AST_PRINTER_H__

#include <iostream>

#include "lval.h"
#include "ast.h"



class ASTPrinter : public ASTVisitor
{
public:
	ASTPrinter(std::ostream& ostr = std::cout);

	virtual void visit(const ASTToken<t_lval>* ast, std::size_t level) const override;
	virtual void visit(const ASTToken<t_real>* ast, std::size_t level) const override;
	virtual void visit(const ASTToken<std::string>* ast, std::size_t level) const override;
	virtual void visit(const ASTToken<void*>* ast, std::size_t level) const override;
	virtual void visit(const ASTDelegate* ast, std::size_t level) const override;
	virtual void visit(const ASTUnary* ast, std::size_t level) const override;
	virtual void visit(const ASTBinary* ast, std::size_t level) const override;


	static std::string get_ast_typename(ASTType ty);


protected:
	void print_base(const ASTBase* ast, std::size_t level,
		const char *extrainfo = nullptr) const;


private:
	std::ostream& m_ostr{std::cout};
};


#endif