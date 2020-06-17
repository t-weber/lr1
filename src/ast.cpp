/**
 * ast
 * @author Tobias Weber
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#include "ast.h"


/**
 * convert concrete syntax tree to abstract one
 */
t_astbaseptr cst_to_ast(t_astbaseptr cst)
{
	if(cst == nullptr)
		return nullptr;

	for(std::size_t i=0; i<cst->NumChildren(); ++i)
		cst->SetChild(i, cst_to_ast(cst->GetChild(i)));

	// skip delegate nodes
	if(cst->GetType() == ASTType::DELEGATE)
		return cst->GetChild(0);

	return cst;
}


std::string get_ast_typename(ASTType ty)
{
	switch(ty)
	{
		case ASTType::TOKEN: return "token";
		case ASTType::DELEGATE: return "delegate";
		case ASTType::UNARY: return "unary";
		case ASTType::BINARY: return "binary";
	}

	return "<unknown>";
}


void ASTBase::print(std::ostream& ostr, std::size_t indent, const char* extrainfo) const
{
	for(std::size_t i=0; i<indent; ++i)
		ostr << "  ";
	ostr << get_ast_typename(GetType()) << ", id=" << GetId();
	//if(GetId() < 256)
	//	ostr << " (" << (char)GetId() << ")";
	if(extrainfo)
		ostr << extrainfo;
	ostr << "\n";

	for(std::size_t i=0; i<NumChildren(); ++i)
		GetChild(i)->print(ostr, indent+1);
}


void ASTUnary::print(std::ostream& ostr, std::size_t indent, const char*) const
{
	std::ostringstream _ostr;
	_ostr << ", op=" << GetOpId();
	if(GetOpId() < 256)
		_ostr << " (" << (char)GetOpId() << ")";
	ASTBase::print(ostr, indent, _ostr.str().c_str());
}


void ASTBinary::print(std::ostream& ostr, std::size_t indent, const char*) const
{
	std::ostringstream _ostr;
	_ostr << ", op=" << GetOpId();
	if(GetOpId() < 256)
		_ostr << " (" << (char)GetOpId() << ")";
	ASTBase::print(ostr, indent, _ostr.str().c_str());
}
