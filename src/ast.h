/**
 * lr(1)
 * @author Tobias Weber
 * @date 14-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __AST_H__
#define __AST_H__

#include <memory>
#include <functional>
#include <optional>


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


private:
	// symbol id (from symbol.h)
	std::size_t m_id;

	// index used in parse tables (from lr1.h)
	std::optional<std::size_t> m_tableidx;
};

using t_astbaseptr = std::shared_ptr<ASTBase>;


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

	t_lval GetLValue() const { return m_lval; }


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


private:
	t_astbaseptr m_arg1, m_arg2;
};



// semantic rule: returns an ast pointer and gets a vector of ast pointers
using t_semanticrule = std::function<t_astbaseptr(const std::vector<t_astbaseptr>&)>;


#endif
