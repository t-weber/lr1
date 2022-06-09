/**
 * ast
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_AST_H__
#define __LR1_AST_H__

#include <memory>
#include <functional>
#include <optional>
#include <iostream>
#include <sstream>

#include "lval.h"


// forward declarations
class ASTBase;
using t_astbaseptr = std::shared_ptr<ASTBase>;

template<class t_lval> class ASTToken;
class ASTDelegate;
class ASTUnary;
class ASTBinary;
class ASTList;


enum class ASTType
{
	TOKEN,
	DELEGATE,
	UNARY,
	BINARY,
	LIST,
};



/**
 * visitor for easier extensibility
 */
class ASTVisitor
{
public:
	virtual ~ASTVisitor() = default;

	//template<class t_lval> virtual void visit(const ASTToken<t_lval>* ast, std::size_t level) const = 0;
	virtual void visit(const ASTToken<t_lval>* ast, std::size_t level) const = 0;
	virtual void visit(const ASTToken<std::string>* ast, std::size_t level) const = 0;
	virtual void visit(const ASTToken<t_real>* ast, std::size_t level) const = 0;
	virtual void visit(const ASTToken<t_int>* ast, std::size_t level) const = 0;
	virtual void visit(const ASTToken<void*>* ast, std::size_t level) const = 0;
	virtual void visit(const ASTDelegate* ast, std::size_t level) const = 0;
	virtual void visit(const ASTUnary* ast, std::size_t level) const = 0;
	virtual void visit(const ASTBinary* ast, std::size_t level) const = 0;
	virtual void visit(const ASTList* ast, std::size_t level) const = 0;
};



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

	virtual void accept(const ASTVisitor* visitor, std::size_t level = 0) const = 0;


	/**
	 * converts the concrete syntax tree to an abstract one
	 */
	static t_astbaseptr cst_to_ast(t_astbaseptr cst)
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


private:
	// symbol id (from symbol.h)
	std::size_t m_id;

	// index used in parse tables (from lr1.h)
	std::optional<std::size_t> m_tableidx;
};



/**
 * visitor acceptor
 */
template<class t_ast_sub>
class ASTBaseAcceptor : public ASTBase
{
public:
	ASTBaseAcceptor(std::size_t id, std::optional<std::size_t> tableidx=std::nullopt)
		: ASTBase{id, tableidx}
	{}

	virtual void accept(const ASTVisitor* visitor, std::size_t level = 0) const override
	{
		const t_ast_sub *sub = static_cast<const t_ast_sub*>(this);
		visitor->visit(sub, level);
	}
};



/**
 * terminal symbols from lexer
 */
template<class t_lval>
class ASTToken : public ASTBaseAcceptor<ASTToken<t_lval>>
{
public:
	ASTToken(std::size_t id, std::size_t tableidx)
		: ASTBaseAcceptor<ASTToken<t_lval>>{id, tableidx}, m_lval{std::nullopt}
	{}

	ASTToken(std::size_t id, std::size_t tableidx, t_lval lval)
		: ASTBaseAcceptor<ASTToken<t_lval>>{id, tableidx}, m_lval{lval}
	{}

	virtual ~ASTToken() = default;

	virtual bool IsTerminal() const override { return true; }
	virtual ASTType GetType() const override { return ASTType::TOKEN; }

	/**
	 * get the lexical value of the token's attribute
	 */
	const t_lval& GetLexerValue() const { return *m_lval; }
	constexpr bool HasLexerValue() const { return m_lval.operator bool(); }


private:
	std::optional<t_lval> m_lval;
};



/**
 * node delegating to one child
 */
class ASTDelegate : public ASTBaseAcceptor<ASTDelegate>
{
public:
	ASTDelegate(std::size_t id, std::size_t tableidx, const t_astbaseptr& arg1)
		: ASTBaseAcceptor<ASTDelegate>{id, tableidx}, m_arg1{arg1}
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
class ASTUnary : public ASTBaseAcceptor<ASTUnary>
{
public:
	ASTUnary(std::size_t id, std::size_t tableidx, const t_astbaseptr& arg1, std::size_t opid)
		: ASTBaseAcceptor<ASTUnary>{id, tableidx}, m_arg1{arg1}, m_opid{opid}
	{}

	virtual ~ASTUnary() = default;

	virtual ASTType GetType() const override { return ASTType::UNARY; }
	std::size_t GetOpId() const { return m_opid; }

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
	std::size_t m_opid;
};



/**
 * node for binary operations
 */
class ASTBinary : public ASTBaseAcceptor<ASTBinary>
{
public:
	ASTBinary(std::size_t id, std::size_t tableidx,
		const t_astbaseptr& arg1, const t_astbaseptr& arg2,
		std::size_t opid)
		: ASTBaseAcceptor<ASTBinary>{id, tableidx}, m_arg1{arg1}, m_arg2{arg2}, m_opid{opid}
	{}

	virtual ~ASTBinary() = default;

	virtual ASTType GetType() const override { return ASTType::BINARY; }
	std::size_t GetOpId() const { return m_opid; }

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
	std::size_t m_opid;
};


/**
 * list node, e.g. for statements
 */
class ASTList : public ASTBaseAcceptor<ASTList>
{
public:
	ASTList(std::size_t id, std::size_t tableidx)
		: ASTBaseAcceptor<ASTList>{id, tableidx}
	{}

	virtual ~ASTList() = default;

	virtual ASTType GetType() const override { return ASTType::LIST; }

	virtual std::size_t NumChildren() const override
	{
		return m_children.size();
	}

	virtual t_astbaseptr GetChild(std::size_t i) const override
	{
		if(i >= m_children.size())
			return nullptr;
		return m_children[i];
	}

	virtual void SetChild(std::size_t i, const t_astbaseptr& ast) override
	{
		if(i >= m_children.size())
			return;
		m_children[i] = ast;
	}

	void AddChild(const t_astbaseptr& ast, bool front = false)
	{
		if(front)
			m_children.insert(m_children.begin(), ast);
		else
			m_children.push_back(ast);
	}


private:
	std::vector<t_astbaseptr> m_children{};
};



// semantic rule: returns an ast pointer and gets a vector of ast pointers
using t_semanticrule = std::function<
	t_astbaseptr(const std::vector<t_astbaseptr>&)>;


#endif
