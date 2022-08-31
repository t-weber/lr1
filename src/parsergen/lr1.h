/**
 * lr(1)
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 *	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 *	- "Übersetzerbau", ISBN: 978-3540653899 (1999, 2013)
 *	- https://www.cs.ecu.edu/karl/5220/spr16/Notes/Bottom-up/lr1.html
 *	- https://www.cs.ecu.edu/karl/5220/spr16/Notes/Bottom-up/slr1table.html
 *	- https://www.cs.uaf.edu/~cs331/notes/FirstFollow.pdf
 *	- https://en.wikipedia.org/wiki/LR_parser
 */

#ifndef __LR1_H__
#define __LR1_H__

#include "symbol.h"
#include "common.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>
#include <functional>
#include <iostream>


class Element;
class Closure;
class Collection;

using ElementPtr = std::shared_ptr<Element>;
using ClosurePtr = std::shared_ptr<Closure>;


enum class ConflictSolution
{
	NONE,
	FORCE_SHIFT,
	FORCE_REDUCE
};


using t_conflictsolution = std::tuple<
	// either use nonterminal lhs or terminal lookback token
	std::variant<NonTerminalPtr, TerminalPtr>,
	// terminal lookahead token
	TerminalPtr,
	ConflictSolution>;



/**
 * LR(1) element
 */
class Element : public std::enable_shared_from_this<Element>
{
public:
	Element(const NonTerminalPtr& lhs, std::size_t rhsidx,
		std::size_t cursor, const Terminal::t_terminalset& la);

	Element(const Element& elem);
	const Element& operator=(const Element& elem);

	NonTerminalPtr GetLhs() const { return m_lhs; }
	const Word* GetRhs() const { return m_rhs; }
	std::optional<std::size_t> GetSemanticRule() const { return m_semanticrule; }

	std::size_t GetCursor() const { return m_cursor; }
	const Terminal::t_terminalset& GetLookaheads() const { return m_lookaheads; }
	WordPtr GetRhsAfterCursor() const;
	SymbolPtr GetSymbolAtCursor() const;

	bool AddLookahead(const TerminalPtr& term);
	bool AddLookaheads(const Terminal::t_terminalset& las);
	void SetLookaheads(const Terminal::t_terminalset& las);

	SymbolPtr GetPossibleTransition() const;

	void AdvanceCursor();
	bool IsCursorAtEnd() const;

	bool IsEqual(const Element& elem, bool only_core=false,
		bool full_equal=true) const;
	bool operator==(const Element& other) const
	{ return IsEqual(other, false); }
	bool operator!=(const Element& other) const
	{ return !operator==(other); }

	std::size_t hash(bool only_core = false) const;

	bool WriteGraphLabel(std::ostream& ostr, bool use_colour=true) const;
	friend std::ostream& operator<<(std::ostream& ostr, const Element& elem);


private:
	NonTerminalPtr m_lhs{nullptr};
	const Word* m_rhs{nullptr};
	std::optional<std::size_t> m_semanticrule{std::nullopt};

	std::size_t m_rhsidx{0};  // rule index
	std::size_t m_cursor{0};  // pointing before element at this index

	Terminal::t_terminalset m_lookaheads{};
};



/**
 * closure of LR(1) element
 */
class Closure : public std::enable_shared_from_this<Closure>
{
public:
	friend class Collection;

	// backtracing of transitions that lead to this closure
	// [ transition terminal, from closure, full lr1 ]
	using t_comefrom_transition = std::tuple<SymbolPtr, ClosurePtr, bool>;

	// hash comefrom transitions
	struct HashComefromTransition
	{
		std::size_t operator()(const t_comefrom_transition& tr) const;
	};

	// compare comefrom transitions for equality
	struct CompareComefromTransitionsEqual
	{
		bool operator()(const t_comefrom_transition& tr1,
			const t_comefrom_transition& tr2) const;
	};

	using t_comefrom_transitions = std::unordered_set<t_comefrom_transition,
		HashComefromTransition, CompareComefromTransitionsEqual>;


public:
	Closure();

	Closure(const Closure& coll);
	const Closure& operator=(const Closure& coll);

	std::size_t GetId() const { return m_id; }

	void AddElement(const ElementPtr& elem);
	std::pair<bool, std::size_t> HasElement(
		const ElementPtr& elem, bool only_core = false) const;

	std::size_t NumElements() const { return m_elems.size(); }
	const ElementPtr& GetElement(std::size_t i) const { return m_elems[i]; }
	ElementPtr GetElementWithCursorAtSymbol(const SymbolPtr& sym) const;

	std::vector<SymbolPtr> GetPossibleTransitions() const;
	ClosurePtr DoTransition(const SymbolPtr&, bool full_lr = true) const;
	std::vector<std::tuple<SymbolPtr, ClosurePtr>> DoTransitions(bool full_lr = true) const;

	bool AddLookaheads(const ClosurePtr& closure);

	std::vector<TerminalPtr> GetComefromTerminals(
		std::shared_ptr<std::unordered_set<std::size_t>> seen_closures = nullptr) const;

	bool HasReduceReduceConflict() const;

	std::size_t hash(bool only_core = false) const;

	void PrintComefroms(std::ostream& ostr) const;

	bool WriteGraphLabel(std::ostream& ostr, bool use_colour=true) const;
	friend std::ostream& operator<<(std::ostream& ostr, const Closure& coll);


private:
	void SetId(std::size_t id) { m_id = id; }


private:
	std::vector<ElementPtr> m_elems{};
	std::size_t m_id{0};      // closure id

	static std::size_t g_id;  // global closure id counter

	// transition that led to this closure
	t_comefrom_transitions m_comefrom_transitions{};
};



/**
 * LR(1) collection of closures
 */
class Collection : public std::enable_shared_from_this<Collection>
{
public:
	// transition [ from closure, to closure, symbol, full_lr1 ]
	using t_transition = std::tuple<ClosurePtr, ClosurePtr, SymbolPtr, bool>;

	// hash transitions
	struct HashTransition
	{
		std::size_t operator()(const t_transition& tr) const;
	};

	// compare transitions for equality
	struct CompareTransitionsEqual
	{
		bool operator()(const t_transition& tr1, const t_transition& tr2) const;
	};

	// compare transitions by order
	struct CompareTransitionsLess
	{
		bool operator()(const t_transition& tr1, const t_transition& tr2) const;
	};

	//using t_transitions = std::set<t_transition, CompareTransitionsLess>;
	using t_transitions = std::unordered_set<t_transition, HashTransition, CompareTransitionsEqual>;
	using t_closurecache = std::shared_ptr<std::unordered_map<std::size_t, ClosurePtr>>;


public:
	Collection(const ClosurePtr& closure);

	void DoTransitions(bool full_lr = true);
	std::tuple<bool, std::size_t> HasReduceReduceConflict() const;
	std::tuple<bool, std::size_t> HasShiftReduceConflict() const;

	Collection ConvertToLALR() const;
	Collection ConvertToSLR(const t_map_follow& follow) const;

	std::tuple<t_table, t_table, t_table,
		t_mapIdIdx, t_mapIdIdx, t_vecIdx> CreateParseTables(
			const std::vector<t_conflictsolution>* conflictsol = nullptr,
			bool stopOnConflicts = true) const;

	static bool SaveParseTables(const std::tuple<t_table, t_table, t_table,
		t_mapIdIdx, t_mapIdIdx, t_vecIdx>& tabs, const std::string& file);

	bool WriteGraph(std::ostream& ostr, bool write_full_coll=true, bool use_colour=true) const;
	bool WriteGraph(const std::string& file, bool write_full_coll=true, bool use_colour=true) const;

	void SetProgressObserver(std::function<void(const std::string&, bool)> func);
	void ReportProgress(const std::string& msg, bool finished = false);


protected:
	Collection();

	void DoTransitions(const ClosurePtr& closure, t_closurecache closure_cache = nullptr);
	void DoLALRTransitions(const ClosurePtr& closure, t_closurecache closure_cache = nullptr);
	void Simplify();

	static std::size_t hash_transition(const t_transition& trans);


private:
	std::vector<ClosurePtr> m_collection{};  // collection of LR(1) closures
	t_transitions m_transitions{};           // transitions between collection, [from, to, transition symbol]

	std::function<void(const std::string& msg, bool finished)> m_progress_observer{};

	friend std::ostream& operator<<(std::ostream& ostr, const Collection& colls);
};

#endif
