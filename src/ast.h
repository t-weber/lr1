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

	virtual void print(std::ostream& ostr, std::size_t indent=0, const char* extrainfo=nullptr) const;


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
	ASTToken(std::size_t id, std::size_t tableidx)
		: ASTBase{id, tableidx}, m_lval{std::nullopt}
	{}

	ASTToken(std::size_t id, std::size_t tableidx, t_lval lval)
		: ASTBase{id, tableidx}, m_lval{lval}
	{}

	virtual ~ASTToken() = default;

	virtual bool IsTerminal() const override { return true; }
	virtual ASTType GetType() const override { return ASTType::TOKEN; }

	/**
	 * get the lexical value of the token's attribute
	 */
	const t_lval& GetLexerValue() const { return *m_lval; }
	constexpr bool HasLexerValue() const { return m_lval.operator bool(); }

	virtual void print(std::ostream& ostr, std::size_t indent=0, const char* =nullptr) const override
	{
		std::ostringstream _ostr;
		if(HasLexerValue())
			_ostr << ", value = " << GetLexerValue();
		ASTBase::print(ostr, indent, _ostr.str().c_str());
	}


private:
	std::optional<t_lval> m_lval;
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
	ASTUnary(std::size_t id, std::size_t tableidx, const t_astbaseptr& arg1, std::size_t opid)
	: ASTBase{id, tableidx}, m_arg1{arg1}, m_opid{opid}
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

	virtual void print(std::ostream& ostr, std::size_t indent=0, const char* =nullptr) const override;

private:
	t_astbaseptr m_arg1;
	std::size_t m_opid;
};


/**
 * node for binary operations
 */
class ASTBinary : public ASTBase
{
public:
	ASTBinary(std::size_t id, std::size_t tableidx,
		const t_astbaseptr& arg1, const t_astbaseptr& arg2,
		std::size_t opid)
		: ASTBase{id, tableidx}, m_arg1{arg1}, m_arg2{arg2}, m_opid{opid}
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

	virtual void print(std::ostream& ostr, std::size_t indent=0, const char* =nullptr) const override;

private:
	t_astbaseptr m_arg1, m_arg2;
	std::size_t m_opid;
};


/**
 * convert concrete syntax tree to abstract one
 */
extern t_astbaseptr cst_to_ast(t_astbaseptr cst);


// semantic rule: returns an ast pointer and gets a vector of ast pointers
using t_semanticrule = std::function<
	t_astbaseptr(const std::vector<t_astbaseptr>&)>;


#endif
