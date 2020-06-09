/**
 * lr(1)
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 *	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 *  - "Übersetzerbau" ISBN: 978-3540653899 (1999, 2013)
 */

#include "lr1.h"


/**
 * comparator for lookaheads set
 */
std::function<bool(const TerminalPtr term1, const TerminalPtr term2)>
Element::lookaheads_compare = [](const TerminalPtr term1, const TerminalPtr term2) -> bool
{
	const std::string& id1 = term1->GetId();
	const std::string& id2 = term2->GetId();

	return std::lexicographical_compare(id1.begin(), id1.end(), id2.begin(), id2.end());
};


Element::Element(const NonTerminalPtr lhs, std::size_t rhsidx, std::size_t cursor, const t_lookaheads& la)
	: m_lhs{lhs}, m_rhs{&lhs->GetRule(rhsidx)}, m_rhsidx{rhsidx}, m_cursor{cursor}, m_lookaheads{la}
{
}


Element::Element(const Element& elem)
{
	this->operator=(elem);
}


const Element& Element::operator=(const Element& elem)
{
	this->m_lhs = elem.m_lhs;
	this->m_rhs = elem.m_rhs;
	this->m_rhsidx = elem.m_rhsidx;
	this->m_cursor = elem.m_cursor;
	this->m_lookaheads = elem.m_lookaheads;

	return *this;
}


bool Element::IsEqual(const Element& elem, bool only_core) const
{
	if(this->GetLhs()->GetId() != elem.GetLhs()->GetId())
		return false;
	if(*this->GetRhs() != *elem.GetRhs())
		return false;
	if(this->GetCursor() != elem.GetCursor())
		return false;

	// also compare lookaheads
	if(!only_core)
	{
		// exact match
		//if(this->GetLookaheads() != elem.GetLookaheads())
		//	return false;

		// see if all lookaheads of elem are already in this lookahead set
		for(const TerminalPtr la : elem.GetLookaheads())
		{
			if(!this->GetLookaheads().contains(la))
				return false;
		}
	}

	return true;
}


WordPtr Element::GetRhsAfterCursor() const
{
	WordPtr rule = std::make_shared<Word>();

	for(std::size_t i=GetCursor()+1; i<GetRhs()->size(); ++i)
		rule->AddSymbol(GetRhs()->GetSymbol(i));

	return rule;
}


void Element::AddLookahead(TerminalPtr term)
{
	m_lookaheads.insert(term);
}


void Element::AddLookaheads(const t_lookaheads& las)
{
	for(TerminalPtr la : las)
		AddLookahead(la);
}


std::ostream& operator<<(std::ostream& ostr, const Element& elem)
{
	const NonTerminalPtr lhs = elem.GetLhs();
	const Word* rhs = elem.GetRhs();

	ostr << lhs->GetId() << " -> [ ";
	for(std::size_t i=0; i<rhs->size(); ++i)
	{
		if(elem.GetCursor() == i)
			ostr << ".";

		const SymbolPtr sym = (*rhs)[i];
		ostr << sym->GetId() << " ";
	}

	ostr << ", ";

	for(const auto& la : elem.GetLookaheads())
		ostr << la->GetId() << " ";

	ostr << "]";

	return ostr;
}



// ----------------------------------------------------------------------------


/**
 * adds an element and generates the rest of the collection
 */
void Collection::AddElement(const ElementPtr elem)
{
	// full element already in collection?
	if(HasElement(elem, false).first)
		return;

	// core element already in collection?
	auto [core_in_coll, core_idx] = HasElement(elem, true);
	if(core_in_coll)
	{
		// add new lookahead
		m_elems[core_idx]->AddLookaheads(elem->GetLookaheads());
		//return;
	}
	// new element
	else
	{
		m_elems.push_back(elem);
	}


	// if the cursor is before a non-terminal, add the rule as element
	const Word* rhs = elem->GetRhs();
	std::size_t cursor = elem->GetCursor();
	if(cursor < rhs->size() && !(*rhs)[cursor]->IsTerminal())
	{
		// get rest of the rule after the cursor and lookaheads
		WordPtr ruleaftercursor = elem->GetRhsAfterCursor();
		const Element::t_lookaheads& nonterm_la = elem->GetLookaheads();

		// get non-terminal at cursor
		NonTerminalPtr nonterm = std::dynamic_pointer_cast<NonTerminal>((*rhs)[cursor]);

		// iterate all rules of the non-terminal
		for(std::size_t nonterm_rhsidx=0; nonterm_rhsidx<nonterm->NumRules(); ++nonterm_rhsidx)
		{
			// iterate lookaheads
			for(const TerminalPtr la : nonterm_la)
			{
				// copy ruleaftercursor and add lookahead
				WordPtr _ruleaftercursor = std::make_shared<Word>(*ruleaftercursor);
				_ruleaftercursor->AddSymbol(la);

				NonTerminalPtr tmpNT = std::make_shared<NonTerminal>("tmp");
				tmpNT->AddRule(*_ruleaftercursor);

				std::map<std::string, std::set<TerminalPtr>> tmp_first;
				calc_first(tmpNT, tmp_first);

				Element::t_lookaheads first_la{Element::lookaheads_compare};
				if(tmp_first.size())	// should always be 1
				{
					const std::set<TerminalPtr>& set_first = tmp_first.begin()->second;
					if(set_first.size())	// should always be 1
						first_la.insert(*set_first.begin());
				}

				AddElement(std::make_shared<Element>(nonterm, nonterm_rhsidx, 0, first_la));
			}
		}
	}
}



/**
 * checks if an element is already in the collection and returns its index
 */
std::pair<bool, std::size_t> Collection::HasElement(const ElementPtr elem, bool only_core) const
{
	for(std::size_t idx=0; idx<m_elems.size(); ++idx)
	{
		const ElementPtr theelem = m_elems[idx];

		if(theelem->IsEqual(*elem, only_core))
			return std::make_pair(true, idx);
	}

	return std::make_pair(false, 0);
}


std::ostream& operator<<(std::ostream& ostr, const Collection& coll)
{
	for(std::size_t i=0; i<coll.NumElements(); ++i)
		ostr << *coll.GetElement(i)<< "\n";

	return ostr;
}
