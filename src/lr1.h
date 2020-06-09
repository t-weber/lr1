/**
 * lr(1)
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_H__
#define __LR1_H__

#include "symbol.h"

#include <set>
#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>


/**
 * LR(1) element
 */
class Element
{
public:
	static std::function<bool(const TerminalPtr term1, const TerminalPtr term2)> lookaheads_compare;
	using t_lookaheads = std::set<TerminalPtr, decltype(lookaheads_compare)>;


public:
	Element(const NonTerminalPtr lhs, std::size_t rhsidx, std::size_t cursor, const t_lookaheads& la);

	Element(const Element& elem);
	const Element& operator=(const Element& elem);

	const NonTerminalPtr GetLhs() const { return m_lhs; }
	const Word* GetRhs() const { return m_rhs; }
	std::size_t GetCursor() const { return m_cursor; }
	const t_lookaheads& GetLookaheads() const { return m_lookaheads; }
	WordPtr GetRhsAfterCursor() const;

	void AddLookahead(TerminalPtr term);
	void AddLookaheads(const t_lookaheads& las);

	bool IsEqual(const Element& elem, bool only_core=false) const;
	bool operator==(const Element& other) const { return IsEqual(other, false); }
	bool operator!=(const Element& other) const { return !operator==(other); }

	friend std::ostream& operator<<(std::ostream& ostr, const Element& elem);


private:
	NonTerminalPtr m_lhs = nullptr;
	const Word* m_rhs = nullptr;
	std::size_t m_rhsidx = 0;	// rule index
	std::size_t m_cursor = 0;	// pointing before element at this index

	t_lookaheads m_lookaheads{lookaheads_compare};
};


using ElementPtr = std::shared_ptr<Element>;



/**
 * LR(1) collection
 */
class Collection
{
public:
	Collection() : m_elems{}
	{}

	void AddElement(const ElementPtr elem);
	std::pair<bool, std::size_t> HasElement(const ElementPtr elem, bool only_core=false) const;

	std::size_t NumElements() const { return m_elems.size(); }
	const ElementPtr GetElement(std::size_t i) const { return m_elems[i]; }

	friend std::ostream& operator<<(std::ostream& ostr, const Collection& coll);

private:
	std::vector<ElementPtr> m_elems;
};


#endif
