/**
 * lr(1)
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_H__
#define __LR1_H__

#include "symbol.h"
#include "common.h"

#include <set>
#include <map>
#include <vector>
#include <functional>
#include <iostream>


class Element;
class Closure;
class Collection;
using ElementPtr = std::shared_ptr<Element>;
using ClosurePtr = std::shared_ptr<Closure>;


enum class ConflictSolution
{
	FORCE_SHIFT,
	FORCE_REDUCE
};


using t_conflictsolution = std::tuple<
	NonTerminalPtr,
	TerminalPtr,
	ConflictSolution>;



/**
 * LR(1) element
 */
class Element
{
public:
	Element(const NonTerminalPtr lhs, std::size_t rhsidx, std::size_t cursor, const Terminal::t_terminalset& la);

	Element(const Element& elem);
	const Element& operator=(const Element& elem);

	const NonTerminalPtr GetLhs() const { return m_lhs; }
	const Word* GetRhs() const { return m_rhs; }
	std::optional<std::size_t> GetSemanticRule() const { return m_semanticrule; }

	std::size_t GetCursor() const { return m_cursor; }
	const Terminal::t_terminalset& GetLookaheads() const { return m_lookaheads; }
	WordPtr GetRhsAfterCursor() const;
	const SymbolPtr GetSymbolAtCursor() const;

	void AddLookahead(TerminalPtr term);
	void AddLookaheads(const Terminal::t_terminalset& las);
	void SetLookaheads(const Terminal::t_terminalset& las);

	const SymbolPtr GetPossibleTransition() const;
	void AdvanceCursor();
	bool IsCursorAtEnd() const;

	bool IsEqual(const Element& elem, bool only_core=false, bool full_equal=true) const;
	bool operator==(const Element& other) const { return IsEqual(other, false); }
	bool operator!=(const Element& other) const { return !operator==(other); }

	std::size_t hash(bool only_core=false) const;

	friend std::ostream& operator<<(std::ostream& ostr, const Element& elem);


private:
	NonTerminalPtr m_lhs = nullptr;
	const Word* m_rhs = nullptr;
	std::optional<std::size_t> m_semanticrule = std::nullopt;

	std::size_t m_rhsidx = 0;	// rule index
	std::size_t m_cursor = 0;	// pointing before element at this index

	Terminal::t_terminalset m_lookaheads;
};



/**
 * closure of LR(1) element
 */
class Closure
{
	friend class Collection;

public:
	Closure();

	Closure(const Closure& coll);
	const Closure& operator=(const Closure& coll);

	std::size_t GetId() const { return m_id; }

	void AddElement(const ElementPtr elem);
	std::pair<bool, std::size_t> HasElement(const ElementPtr elem, bool only_core=false) const;

	std::size_t NumElements() const { return m_elems.size(); }
	const ElementPtr GetElement(std::size_t i) const { return m_elems[i]; }
	const ElementPtr GetElementWithCursorAtSymbol(const SymbolPtr& sym) const;

	std::vector<SymbolPtr> GetPossibleTransitions() const;
	ClosurePtr DoTransition(const SymbolPtr) const;
	std::vector<std::tuple<SymbolPtr, ClosurePtr>> DoTransitions() const;

	std::size_t hash(bool only_core=false) const;

	friend std::ostream& operator<<(std::ostream& ostr, const Closure& coll);


private:
	void SetId(std::size_t id) { m_id = id; }


private:
	std::vector<ElementPtr> m_elems;
	std::size_t m_id = 0;	// Closure id

	// global Closure id counter
	static std::size_t g_id;
};



/**
 * LR(1) collection
 */
class Collection
{
public:
	// transition from closure 1 to closure 2 with a symbol
	using t_transition = std::tuple<ClosurePtr, ClosurePtr, SymbolPtr>;


public:
	Collection(const ClosurePtr coll);

	void DoTransitions();

	Collection ConvertToLALR() const;
	Collection ConvertToSLR(const t_map_follow& follow) const;

	std::tuple<t_table, t_table, t_table,
		t_mapIdIdx, t_mapIdIdx, t_vecIdx> CreateParseTables(
			const std::vector<t_conflictsolution>* conflictsol = nullptr) const;

	static bool SaveParseTables(const std::tuple<t_table, t_table, t_table,
		t_mapIdIdx, t_mapIdIdx, t_vecIdx>& tabs, const std::string& file);

	bool WriteGraph(const std::string& file, bool write_full_coll=1) const;


protected:
	Collection();

	void DoTransitions(const ClosurePtr coll);
	void Simplify();

	static std::size_t hash_transition(const t_transition& trans);


private:
	std::map<std::size_t, ClosurePtr> m_cache;	// closure hashes
	std::vector<ClosurePtr> m_collection;		// collection

	// transitions between collection, [from, to, transition symbol]
	std::vector<t_transition> m_transitions;

	friend std::ostream& operator<<(std::ostream& ostr, const Collection& colls);
};

#endif
