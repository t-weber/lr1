/**
 * ll(1)
 * @author Tobias Weber
 * @date 27-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 *	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 *	- "Übersetzerbau", ISBN: 978-3540653899 (1999, 2013)
 */

#include "ll1.h"


void LL1::CalcFirstFollow()
{
	// first
	for(const NonTerminalPtr& nonterminal : m_nonterminals)
		calc_first(nonterminal, m_first, &m_first_per_rule);

	// follow
	for(const NonTerminalPtr& nonterminal : m_nonterminals)
		calc_follow(m_nonterminals, m_start, nonterminal, m_first, m_follow);
}


void LL1::RemoveLeftRecursion(
	std::size_t newIdBegin, const std::string& primerule,
	std::size_t *semanticruleidx)
{
	std::vector<NonTerminalPtr> newnonterminals;
	for(NonTerminalPtr ptr : m_nonterminals)
	{
		NonTerminalPtr newptr = ptr->RemoveLeftRecursion(newIdBegin, primerule, semanticruleidx);
		if(newptr)
			newnonterminals.emplace_back(newptr);
	}

	// add new primed rules
	m_nonterminals.insert(m_nonterminals.end(), newnonterminals.begin(), newnonterminals.end());
}


/**
 * calculate the ll(1) parsing table
 */
void LL1::CalcTable()
{
}


/**
 * pseudocode for ll(1) recursive descent parsing
 */
void LL1::PrintRecursiveDescentPseudocode() const
{
}


/**
 * print ll(1) infos
 */
std::ostream& operator<<(std::ostream& ostr, const LL1& ll1)
{
	ostr << "Productions:\n";
	for(NonTerminalPtr nonterm : ll1.GetNonTerminals())
		nonterm->print(ostr);
	ostr << std::endl;

	ostr << "FIRST sets:\n";
	for(const auto& pair : ll1.GetFirst())
	{
		ostr << pair.first->GetStrId() << ": ";
		for(const auto& _first : pair.second)
			ostr << _first->GetStrId() << ", ";
		ostr << "\n";
	}
	ostr << "\n";


	ostr << "FOLLOW sets:\n";
	for(const auto& pair : ll1.GetFollow())
	{
		ostr << pair.first->GetStrId() << ": ";
		for(const auto& _first : pair.second)
			ostr << _first->GetStrId() << ", ";
		ostr << "\n";
	}

	return ostr;
}
