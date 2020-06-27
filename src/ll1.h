/**
 * ll(1)
 * @author Tobias Weber
 * @date 27-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LL1_H__
#define __LL1_H__

#include "symbol.h"

#include <iostream>


class LL1
{
public:
	LL1() = default;
	~LL1() = default;

	LL1(const std::initializer_list<NonTerminalPtr>& nonterminals, NonTerminalPtr start=nullptr)
		: m_nonterminals{nonterminals}, m_start(start), m_first{}, m_first_per_rule{}, m_follow{}
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
	void PrintRecursiveDescentPseudocode() const;

	const std::vector<NonTerminalPtr>& GetNonTerminals() const { return m_nonterminals; }
	const t_map_first& GetFirst() const { return m_first; }
	const t_map_follow& GetFollow() const { return m_follow; }

	friend std::ostream& operator<<(std::ostream& ostr, const LL1& ll1);


private:
	std::vector<NonTerminalPtr> m_nonterminals;
	NonTerminalPtr m_start = nullptr;

	t_map_first m_first;
	t_map_first_perrule m_first_per_rule;

	t_map_follow m_follow;
};

#endif
