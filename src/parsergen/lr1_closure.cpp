/**
 * lr(1) closure
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


// global closure id counter
std::size_t Closure::g_id = 0;


Closure::Closure() : m_elems{}, m_id{g_id++}
{
}


Closure::Closure(const Closure& closure) : m_elems{}
{
	this->operator=(closure);
}


const Closure& Closure::operator=(const Closure& closure)
{
	this->m_id = closure.m_id;
	this->m_comefrom_transitions = closure.m_comefrom_transitions;

	for(ElementPtr elem : closure.m_elems)
		this->m_elems.emplace_back(std::make_shared<Element>(*elem));

	return *this;
}


/**
 * adds an element and generates the rest of the closure
 */
void Closure::AddElement(const ElementPtr elem)
{
	//std::cout << "adding " << *elem << std::endl;

	// full element already in Closure?
	if(HasElement(elem, false).first)
		return;

	// core element already in Closure?
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
		const Terminal::t_terminalset& nonterm_la = elem->GetLookaheads();

		// get non-terminal at cursor
		NonTerminalPtr nonterm = std::dynamic_pointer_cast<NonTerminal>((*rhs)[cursor]);

		// iterate all rules of the non-terminal
		for(std::size_t nonterm_rhsidx=0; nonterm_rhsidx<nonterm->NumRules(); ++nonterm_rhsidx)
		{
			// iterate lookaheads
			for(const TerminalPtr& la : nonterm_la)
			{
				// copy ruleaftercursor and add lookahead
				WordPtr _ruleaftercursor = std::make_shared<Word>(*ruleaftercursor);
				_ruleaftercursor->AddSymbol(la);
				//std::cout << nonterm->GetId() << ", " << *_ruleaftercursor << std::endl;

				NonTerminalPtr tmpNT = std::make_shared<NonTerminal>(0, "tmp");
				tmpNT->AddRule(*_ruleaftercursor);

				t_map_first tmp_first;
				calc_first(tmpNT, tmp_first);

				Terminal::t_terminalset first_la;
				if(tmp_first.size())	// should always be 1
				{
					const Terminal::t_terminalset& set_first = tmp_first.begin()->second;
					if(set_first.size())	// should always be 1
					{
						TerminalPtr la = *set_first.begin();
						//std::cout << "lookahead: " << la->GetId() << std::endl;
						first_la.insert(la);
					}
				}

				AddElement(std::make_shared<Element>(nonterm, nonterm_rhsidx, 0, first_la));
			}
		}
	}
}


/**
 * checks if an element is already in the closure and returns its index
 */
std::pair<bool, std::size_t> Closure::HasElement(const ElementPtr elem, bool only_core) const
{
	for(std::size_t idx=0; idx<m_elems.size(); ++idx)
	{
		const ElementPtr theelem = m_elems[idx];

		if(theelem->IsEqual(*elem, only_core, false))
			return std::make_pair(true, idx);
	}

	return std::make_pair(false, 0);
}


/**
 * get the element of the collection whose cursor points to the given symbol
 */
const ElementPtr Closure::GetElementWithCursorAtSymbol(const SymbolPtr& sym) const
{
	for(std::size_t idx=0; idx<m_elems.size(); ++idx)
	{
		const ElementPtr theelem = m_elems[idx];

		const Word* rhs = theelem->GetRhs();
		if(!rhs)
			continue;
		std::size_t cursor = theelem->GetCursor();

		if(cursor >= rhs->NumSymbols())
			continue;

		if(rhs->GetSymbol(cursor)->GetId() == sym->GetId())
			return theelem;
	}

	return nullptr;
}


/**
 * get possible transition symbols from all elements
 */
std::vector<SymbolPtr> Closure::GetPossibleTransitions() const
{
	std::vector<SymbolPtr> syms;

	for(const ElementPtr& theelem : m_elems)
	{
		const SymbolPtr sym = theelem->GetPossibleTransition();
		if(!sym)
			continue;

		bool sym_already_seen = std::find_if(syms.begin(), syms.end(),
			[sym](const SymbolPtr sym2) -> bool
			{
				return *sym == *sym2;
			}) != syms.end();

		if(sym && !sym_already_seen)
			syms.emplace_back(sym);
	}

	return syms;
}


/**
 * perform a transition and get the corresponding lr(1) closure
 */
ClosurePtr Closure::DoTransition(const SymbolPtr transsym) const
{
	ClosurePtr newclosure = std::make_shared<Closure>();

	// look for elements with that transition
	for(const ElementPtr& theelem : m_elems)
	{
		const SymbolPtr sym = theelem->GetPossibleTransition();
		if(!sym || *sym != *transsym)
			continue;

		// copy element and perform transition
		ElementPtr newelem = std::make_shared<Element>(*theelem);
		newelem->AdvanceCursor();

		newclosure->AddElement(newelem);
		newclosure->AddComefromTransition(std::make_tuple(transsym, this));
	}

	return newclosure;
}


/**
 * perform all possible transitions from this closure and get the corresponding lr(1) collection
 * @return [transition symbol, destination closure]
 */
std::vector<std::tuple<SymbolPtr, ClosurePtr>> Closure::DoTransitions() const
{
	std::vector<std::tuple<SymbolPtr, ClosurePtr>> coll;
	std::vector<SymbolPtr> possible_transitions = GetPossibleTransitions();

	for(const SymbolPtr& transition : possible_transitions)
	{
		ClosurePtr closure = DoTransition(transition);
		coll.emplace_back(std::make_tuple(transition, closure));
	}

	return coll;
}


std::size_t Closure::hash(bool only_core) const
{
	// sort element hashes before combining them
	std::vector<std::size_t> hashes;
	hashes.reserve(m_elems.size());

	for(ElementPtr elem : m_elems)
		hashes.emplace_back(elem->hash(only_core));

	std::sort(hashes.begin(), hashes.end(), [](std::size_t hash1, std::size_t hash2) -> bool
	{
		return hash1 < hash2;
	});


	std::size_t fullhash = 0;
	for(std::size_t hash : hashes)
		boost::hash_combine(fullhash, hash);
	return fullhash;
}


/**
 * adds a reference to where a possible transition can come from
 */
void Closure::AddComefromTransition(const t_comefrom_transition& comefrom)
{
	// do we already have this comefrom?
	for(const t_comefrom_transition& oldcomefrom : m_comefrom_transitions)
	{
		if(*std::get<0>(comefrom) == *std::get<0>(oldcomefrom) &&
			std::get<1>(comefrom)->GetId() == std::get<1>(oldcomefrom)->GetId())
			return;
	}

	// add unique comefrom
	m_comefrom_transitions.push_back(comefrom);
}


/**
 * get all terminal symbols that lead to this closure
 */
std::vector<TerminalPtr> Closure::GetComefromTerminals() const
{
	std::vector<TerminalPtr> terms;

	for(const t_comefrom_transition& comefrom : m_comefrom_transitions)
	{
		SymbolPtr sym = std::get<0>(comefrom);
		const Closure* closure = std::get<1>(comefrom);

		if(sym->IsTerminal())
		{
			TerminalPtr term = std::dynamic_pointer_cast<Terminal>(sym);
			terms.emplace_back(std::move(term));
		}
		else if(closure)
		{
			// get terminals from comefrom closure
			std::vector<TerminalPtr> _terms = closure->GetComefromTerminals();
			terms.insert(terms.end(), _terms.begin(), _terms.end());
		}
	}

	// remove duplicates
	auto end = std::unique(terms.begin(), terms.end(),
	[](TerminalPtr term1, TerminalPtr term2) -> bool
	{
		return *term1 == *term2;
	});
	if(end != terms.end())
		terms.resize(end - terms.begin());

	return terms;
}


/**
 * removes duplicate comefrom transitions
 */
void Closure::CleanComefromTransitions()
{
	// remove duplicates
	auto end = std::unique(m_comefrom_transitions.begin(), m_comefrom_transitions.end(),
	[](const t_comefrom_transition& comefrom1, const t_comefrom_transition& comefrom2) -> bool
	{
		return *std::get<0>(comefrom1) == *std::get<0>(comefrom2) &&
			std::get<1>(comefrom1)->GetId() == std::get<1>(comefrom2)->GetId();
	});

	if(end != m_comefrom_transitions.end())
		m_comefrom_transitions.resize(end-m_comefrom_transitions.begin());
}


/**
 * prints a closure
 */
std::ostream& operator<<(std::ostream& ostr, const Closure& closure)
{
	ostr << "Closure/State " << closure.GetId() << ":\n";

	// write elements of the closure
	for(std::size_t i=0; i<closure.NumElements(); ++i)
		ostr << "\t" << *closure.GetElement(i)<< "\n";

	// write the closures where we can come from
	const auto& comefroms = closure.GetComefromTransitions();
	if(comefroms.size())
	{
		ostr << "Coming from possible transitions:\n";
		for(std::size_t i=0; i<comefroms.size(); ++i)
		{
			const Closure::t_comefrom_transition& comefrom = comefroms[i];
			ostr << "\tstate " << std::get<1>(comefrom)->GetId()
				<< " via " << std::get<0>(comefrom)->GetStrId()
				<< "." << std::endl;
		}
	}

	return ostr;
}
