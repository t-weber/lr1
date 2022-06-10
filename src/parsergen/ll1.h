/**
 * ll(1)
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 27-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LL1_H__
#define __LL1_H__

#include "symbol.h"

#include <map>
#include <iostream>


class LL1
{
public:
	// table[nonterminal, terminal] = rule
	using t_map_terms = std::map<TerminalPtr, std::size_t, Symbol::CompareSymbols>;
	using t_map_ll1 = std::map<NonTerminalPtr, t_map_terms, Symbol::CompareSymbols>;


public:
	LL1() = default;
	~LL1() = default;

	LL1(const std::initializer_list<NonTerminalPtr>& nonterminals, NonTerminalPtr start=nullptr)
		: m_nonterminals{nonterminals}, m_start(start), m_first{}, m_first_per_rule{}, m_follow{},
			m_table{}
	{
		// if no start symbol is given, use first one
		if(m_start == nullptr && m_nonterminals.size())
			m_start = m_nonterminals[0];
	}

	void CalcFirstFollow();
	void RemoveLeftRecursion(
		std::size_t newIdBegin = 1000, const std::string& primerule = "'",
		std::size_t* semanticruleidx = nullptr);
	void CalcTable();
	void PrintRecursiveDescentPseudocode(std::ostream& ostr = std::cout) const;

	const std::vector<NonTerminalPtr>& GetNonTerminals() const
	{ return m_nonterminals; }
	const t_map_first& GetFirst() const { return m_first; }
	const t_map_follow& GetFollow() const { return m_follow; }
	const t_map_ll1& GetTable() const { return m_table; }

	friend std::ostream& operator<<(std::ostream& ostr, const LL1& ll1);


protected:
	static Terminal::t_terminalset GetLookaheads(
		const Terminal::t_terminalset& first,
		const Terminal::t_terminalset& follow);


private:
	std::vector<NonTerminalPtr> m_nonterminals{};
	NonTerminalPtr m_start{nullptr};

	t_map_first m_first{};
	t_map_first_perrule m_first_per_rule{};

	t_map_follow m_follow{};

	t_map_ll1 m_table{};
};

#endif
