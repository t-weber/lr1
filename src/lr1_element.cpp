/**
 * lr(1) element
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 *	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 *	- "Übersetzerbau", ISBN: 978-3540653899 (1999, 2013)
 */

#include "lr1.h"

#include <sstream>
#include <algorithm>

#include <boost/functional/hash.hpp>


Element::Element(const NonTerminalPtr lhs, std::size_t rhsidx, std::size_t cursor, const Terminal::t_terminalset& la)
	: m_lhs{lhs}, m_rhs{&lhs->GetRule(rhsidx)}, m_semanticrule{lhs->GetSemanticRule(rhsidx)},
		m_rhsidx{rhsidx}, m_cursor{cursor}, m_lookaheads{la}
{
}


Element::Element(const Element& elem) : m_lookaheads{}
{
	this->operator=(elem);
}


const Element& Element::operator=(const Element& elem)
{
	this->m_lhs = elem.m_lhs;
	this->m_rhs = elem.m_rhs;
	this->m_semanticrule = elem.m_semanticrule;
	this->m_rhsidx = elem.m_rhsidx;
	this->m_cursor = elem.m_cursor;
	this->m_lookaheads = elem.m_lookaheads;

	return *this;
}


bool Element::IsEqual(const Element& elem, bool only_core, bool full_equal) const
{
	if(*this->GetLhs() != *elem.GetLhs())
		return false;
	if(*this->GetRhs() != *elem.GetRhs())
		return false;
	if(this->GetCursor() != elem.GetCursor())
		return false;

	// also compare lookaheads
	if(!only_core)
	{
		if(full_equal)
		{
			// exact match
			if(this->GetLookaheads() != elem.GetLookaheads())
				return false;
		}
		else
		{
			// see if all lookaheads of elem are already in this lookahead set
			for(const TerminalPtr& la : elem.GetLookaheads())
			{
				if(this->GetLookaheads().find(la) == this->GetLookaheads().end())
					return false;
			}
		}
	}

	return true;
}


std::size_t Element::hash(bool only_core) const
{
	std::size_t hashLhs = this->GetLhs()->hash();
	std::size_t hashRhs = this->GetRhs()->hash();
	std::size_t hashCursor = std::hash<std::size_t>{}(this->GetCursor());

	boost::hash_combine(hashLhs, hashRhs);
	boost::hash_combine(hashLhs, hashCursor);

	if(!only_core)
	{
		for(const TerminalPtr& la : GetLookaheads())
		{
			std::size_t hashLA = la->hash();
			boost::hash_combine(hashLhs, hashLA);
		}
	}

	return hashLhs;
}


WordPtr Element::GetRhsAfterCursor() const
{
	WordPtr rule = std::make_shared<Word>();

	for(std::size_t i=GetCursor()+1; i<GetRhs()->size(); ++i)
		rule->AddSymbol(GetRhs()->GetSymbol(i));

	return rule;
}


const SymbolPtr Element::GetSymbolAtCursor() const
{
	const Word* rhs = GetRhs();
	if(!rhs)
		return nullptr;

	std::size_t cursor = GetCursor();
	if(cursor >= rhs->NumSymbols())
		return nullptr;

	return rhs->GetSymbol(cursor);
}


void Element::AddLookahead(TerminalPtr term)
{
	m_lookaheads.insert(term);
}


void Element::AddLookaheads(const Terminal::t_terminalset& las)
{
	for(TerminalPtr la : las)
		AddLookahead(la);
}


void Element::SetLookaheads(const Terminal::t_terminalset& las)
{
	m_lookaheads = las;
}


/**
 * get possible transition symbol
 */
const SymbolPtr Element::GetPossibleTransition() const
{
	if(m_cursor >= m_rhs->size())
		return nullptr;
	SymbolPtr sym = (*m_rhs)[m_cursor];
	if(sym->IsEps())
		return nullptr;
	return sym;
}


void Element::AdvanceCursor()
{
	if(m_cursor < m_rhs->size())
		++m_cursor;
}


/**
 * cursor is at end, i.e. full handle has been read and can be reduced
 */
bool Element::IsCursorAtEnd() const
{
	return m_cursor >= m_rhs->size();
}


std::ostream& operator<<(std::ostream& ostr, const Element& elem)
{
	const NonTerminalPtr lhs = elem.GetLhs();
	const Word* rhs = elem.GetRhs();

	ostr << lhs->GetStrId() << " -> [ ";
	for(std::size_t i=0; i<rhs->size(); ++i)
	{
		if(elem.GetCursor() == i)
			ostr << ".";

		const SymbolPtr sym = (*rhs)[i];

		ostr << sym->GetStrId();
		//if(i < rhs->size()-1)
			ostr << " ";
	}

	// cursor at end?
	if(elem.GetCursor() == rhs->size())
		ostr << ".";

	ostr << ", ";

	for(const auto& la : elem.GetLookaheads())
		ostr << la->GetStrId() << " ";

	ostr << "]";

	return ostr;
}
