/**
 * ast
 * @author Tobias Weber
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __AST_H__
#define __AST_H__

#include <memory>
#include <functional>
#include <optional>
#include <iostream>


class ASTBase;
using t_astbaseptr = std::shared_ptr<ASTBase>;


enum class ASTType
{
	TOKEN,
	DELEGATE,
	UNARY,
	BINARY,
};

extern std::string get_ast_typename(ASTType);


/**
 * syntax tree base
 */
class ASTBase
{
public:
	ASTBase(std::size_t id, std::optional<std::size_t> tableidx=std::nullopt)
		: m_id{id}, m_tableidx{tableidx}
	{}

	virtual ~ASTBase() = default;

	std::size_t GetId() const { return m_id; }
	std::size_t GetTableIdx() const { return *m_tableidx; }
	void SetTableIdx(std::size_t tableidx) { m_tableidx = tableidx; }

	virtual bool IsTerminal() const { return false; };
	virtual ASTType GetType() const = 0;

	virtual std::size_t NumChildren() const { return 0; }
	virtual t_astbaseptr GetChild(std::size_t) const { return nullptr; }
	virtual void SetChild(std::size_t, const t_astbaseptr&) { }

	virtual void print(std::ostream& ostr, std::size_t indent=0)
	{
		for(std::size_t i=0; i<indent; ++i)
			ostr << "  ";
		ostr << get_ast_typename(GetType()) << ", id=" << GetId();
		if(GetId() < 256)
			ostr << " (" << (char)GetId() << ")";
		ostr << "\n";

		for(std::size_t i=0; i<NumChildren(); ++i)
			GetChild(i)->print(ostr, indent+1);
	}


private:
	// symbol id (from symbol.h)
	std::size_t m_id;

	// index used in parse tables (from lr1.h)
	std::optional<std::size_t> m_tableidx;
};


/**
 * terminal symbols from lexer
 */
template<class t_lval>
class ASTToken : public ASTBase
{
public:
	ASTToken(std::size_t id, std::size_t tableidx=0, t_lval lval=t_lval{})
		: ASTBase{id, tableidx}, m_lval{lval}
	{}

	virtual ~ASTToken() = default;

	virtual bool IsTerminal() const override { return true; }
	virtual ASTType GetType() const override { return ASTType::TOKEN; }

	t_lval GetLValue() const { return m_lval; }

	virtual void print(std::ostream& ostr, std::size_t indent=0) override
	{
		for(std::size_t i=0; i<indent; ++i)
			ostr << "  ";
		ostr << get_ast_typename(GetType()) << ", id=" << GetId();
		if(GetId() < 256)
			ostr << " (" << (char)GetId() << ")";
		ostr << ", value = " << std::get<0>(*GetLValue());
		ostr << "\n";
	}


private:
	t_lval m_lval;
};


/**
 * node delegating to one child
 */
class ASTDelegate : public ASTBase
{
public:
	ASTDelegate(std::size_t id, std::size_t tableidx, const t_astbaseptr& arg1)
		: ASTBase{id, tableidx}, m_arg1{arg1}
	{}

	virtual ~ASTDelegate() = default;

	virtual ASTType GetType() const override { return ASTType::DELEGATE; }

	virtual std::size_t NumChildren() const override { return 1; }
	virtual t_astbaseptr GetChild(std::size_t i) const override
	{
		return i==0 ? m_arg1 : nullptr;
	}

	virtual void SetChild(std::size_t i, const t_astbaseptr& ast) override
	{
		if(i==0)
			m_arg1 = ast;
	}


private:
	t_astbaseptr m_arg1;
};


/**
 * node for unary operations
 */
class ASTUnary : public ASTBase
{
public:
	ASTUnary(std::size_t id, std::size_t tableidx, const t_astbaseptr& arg1)
	: ASTBase{id, tableidx}, m_arg1{arg1}
	{}

	virtual ~ASTUnary() = default;

	virtual ASTType GetType() const override { return ASTType::UNARY; }

	virtual std::size_t NumChildren() const override { return 1; }
	virtual t_astbaseptr GetChild(std::size_t i) const override
	{
		return i==0 ? m_arg1 : nullptr;
	}

	virtual void SetChild(std::size_t i, const t_astbaseptr& ast) override
	{
		if(i==0)
			m_arg1 = ast;
	}

private:
	t_astbaseptr m_arg1;
};


/**
 * node for binary operations
 */
class ASTBinary : public ASTBase
{
public:
	ASTBinary(std::size_t id, std::size_t tableidx,
		const t_astbaseptr& arg1, const t_astbaseptr& arg2)
		: ASTBase{id, tableidx}, m_arg1{arg1}, m_arg2{arg2}
	{}

	virtual ~ASTBinary() = default;

	virtual ASTType GetType() const override { return ASTType::BINARY; }

	virtual std::size_t NumChildren() const override { return 2; }
	virtual t_astbaseptr GetChild(std::size_t i) const override
	{
		switch(i)
		{
			case 0: return m_arg1;
			case 1: return m_arg2;
		}

		return nullptr;
	}

	virtual void SetChild(std::size_t i, const t_astbaseptr& ast) override
	{
		switch(i)
		{
			case 0: m_arg1 = ast; break;
			case 1: m_arg2 = ast; break;
		}
	}

private:
	t_astbaseptr m_arg1, m_arg2;
};


/**
 * convert concrete syntax tree to abstract one
 */
extern t_astbaseptr cst_to_ast(t_astbaseptr cst);


// semantic rule: returns an ast pointer and gets a vector of ast pointers
using t_semanticrule = std::function<t_astbaseptr(const std::vector<t_astbaseptr>&)>;


#endif
