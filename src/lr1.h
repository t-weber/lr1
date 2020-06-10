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
#include <map>
#include <vector>
#include <functional>
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

	const SymbolPtr GetPossibleTransition() const;
	void AdvanceCursor();

	bool IsEqual(const Element& elem, bool only_core=false, bool full_equal=true) const;
	bool operator==(const Element& other) const { return IsEqual(other, false); }
	bool operator!=(const Element& other) const { return !operator==(other); }

	std::size_t hash() const;

	friend std::ostream& operator<<(std::ostream& ostr, const Element& elem);


private:
	NonTerminalPtr m_lhs = nullptr;
	const Word* m_rhs = nullptr;
	std::size_t m_rhsidx = 0;	// rule index
	std::size_t m_cursor = 0;	// pointing before element at this index

	t_lookaheads m_lookaheads{lookaheads_compare};
};



class Collection;
using ElementPtr = std::shared_ptr<Element>;
using CollectionPtr = std::shared_ptr<Collection>;



/**
 * LR(1) collection
 */
class Collection
{
public:
	Collection() : m_elems{}, m_id{g_id++}
	{}

	std::size_t GetId() const { return m_id; }

	void AddElement(const ElementPtr elem);
	std::pair<bool, std::size_t> HasElement(const ElementPtr elem, bool only_core=false) const;

	std::size_t NumElements() const { return m_elems.size(); }
	const ElementPtr GetElement(std::size_t i) const { return m_elems[i]; }

	std::vector<SymbolPtr> GetPossibleTransitions() const;
	CollectionPtr DoTransition(const SymbolPtr) const;
	std::vector<std::tuple<SymbolPtr, CollectionPtr>> DoTransitions() const;

	std::size_t hash() const;

	friend std::ostream& operator<<(std::ostream& ostr, const Collection& coll);


private:
	std::vector<ElementPtr> m_elems;
	std::size_t m_id = 0;	// collection id

	// global collection id counter
	static std::size_t g_id;
};



class Collections
{
public:
	Collections(const CollectionPtr coll);
	Collections() = delete;

	void DoTransitions();

public:
	void DoTransitions(const CollectionPtr coll);

private:
	std::map<std::size_t, CollectionPtr> m_cache;	// collection hashes
	std::vector<CollectionPtr> m_collections;		// collections

	// transitions between collections, [from, to, transition symbol]
	std::vector<std::tuple<CollectionPtr, CollectionPtr, SymbolPtr>> m_transitions;

	friend std::ostream& operator<<(std::ostream& ostr, const Collections& colls);
};

#endif
