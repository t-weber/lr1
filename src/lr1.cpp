/**
 * lr(1)
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 *	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 *	- "Übersetzerbau" ISBN: 978-3540653899 (1999, 2013)
 */

#include "lr1.h"

#include <algorithm>
#include <boost/functional/hash.hpp>


/**
 * comparator for lookaheads set
 */
std::function<bool(const TerminalPtr term1, const TerminalPtr term2)>
Element::lookaheads_compare = [](const TerminalPtr term1, const TerminalPtr term2) -> bool
{
	/*
	const std::string& id1 = term1->GetId();
	const std::string& id2 = term2->GetId();
	return std::lexicographical_compare(id1.begin(), id1.end(), id2.begin(), id2.end());
	*/

	return term1->hash() < term2->hash();
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
				if(!this->GetLookaheads().contains(la))
					return false;
			}
		}
	}

	return true;
}


std::size_t Element::hash() const
{
	std::size_t hashLhs = this->GetLhs()->hash();
	std::size_t hashRhs = this->GetLhs()->hash();
	std::size_t hashCursor = std::hash<std::size_t>{}(this->GetCursor());

	boost::hash_combine(hashLhs, hashRhs);
	boost::hash_combine(hashLhs, hashCursor);

	for(const TerminalPtr& la : GetLookaheads())
	{
		std::size_t hashLA = la->hash();
		boost::hash_combine(hashLhs, hashLA);
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


void Element::AddLookahead(TerminalPtr term)
{
	m_lookaheads.insert(term);
}


void Element::AddLookaheads(const t_lookaheads& las)
{
	for(TerminalPtr la : las)
		AddLookahead(la);
}


/**
 * get possible transition symbol
 */
const SymbolPtr Element::GetPossibleTransition() const
{
	if(m_cursor >= m_rhs->size())
		return nullptr;
	return (*m_rhs)[m_cursor];
}


void Element::AdvanceCursor()
{
	if(m_cursor < m_rhs->size())
		++m_cursor;
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

		ostr << sym->GetId();
		//if(i < rhs->size()-1)
			ostr << " ";
	}

	// cursor at end?
	if(elem.GetCursor() == rhs->size())
		ostr << ".";

	ostr << ", ";

	for(const auto& la : elem.GetLookaheads())
		ostr << la->GetId() << " ";

	ostr << "]";

	return ostr;
}



// ----------------------------------------------------------------------------



// global collection id counter
std::size_t Collection::g_id = 0;


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
			for(const TerminalPtr& la : nonterm_la)
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

		if(theelem->IsEqual(*elem, only_core, false))
			return std::make_pair(true, idx);
	}

	return std::make_pair(false, 0);
}


/**
 * get possible transition symbols from all elements
 */
std::vector<SymbolPtr> Collection::GetPossibleTransitions() const
{
	std::vector<SymbolPtr> syms;

	for(const ElementPtr& theelem : m_elems)
	{
		const SymbolPtr sym = theelem->GetPossibleTransition();
		if(!sym)
			continue;

		bool sym_already_seen = std::find_if(syms.begin(), syms.end(), [sym](const SymbolPtr sym2)->bool
		{
			return *sym == *sym2;
		}) != syms.end();
		if(sym && !sym_already_seen)
			syms.emplace_back(sym);
	}

	return syms;
}


/**
 * perform a transition and get the corresponding lr(1) collection
 */
CollectionPtr Collection::DoTransition(const SymbolPtr transsym) const
{
	CollectionPtr newcoll = std::make_shared<Collection>();

	// look for elements with that transition
	for(const ElementPtr& theelem : m_elems)
	{
		const SymbolPtr sym = theelem->GetPossibleTransition();
		if(!sym)
			continue;
		if(*sym != *transsym)
			continue;

		// copy element and perform transition
		ElementPtr newelem = std::make_shared<Element>(*theelem);
		newelem->AdvanceCursor();

		newcoll->AddElement(newelem);
	}

	return newcoll;
}


/**
 * perform all possible transitions from this collection and get the corresponding lr(1) collections
 * @return [transition symbol, destination collection]
 */
std::vector<std::tuple<SymbolPtr, CollectionPtr>> Collection::DoTransitions() const
{
	std::vector<std::tuple<SymbolPtr, CollectionPtr>> colls;
	std::vector<SymbolPtr> possible_transitions = GetPossibleTransitions();

	for(const SymbolPtr& transition : possible_transitions)
	{
		CollectionPtr coll = DoTransition(transition);
		colls.emplace_back(std::make_tuple(transition, coll));
	}

	return colls;
}


std::size_t Collection::hash() const
{
	// sort element hashes before combining them
	std::vector<std::size_t> hashes;
	hashes.reserve(m_elems.size());

	for(ElementPtr elem : m_elems)
		hashes.emplace_back(elem->hash());

	std::sort(hashes.begin(), hashes.end(), [](std::size_t hash1, std::size_t hash2)->bool
	{
		return hash1 < hash2;
	});


	std::size_t fullhash = 0;
	for(std::size_t hash : hashes)
		boost::hash_combine(fullhash, hash);
	return fullhash;
}


std::ostream& operator<<(std::ostream& ostr, const Collection& coll)
{
	ostr << "Collection " << coll.GetId() << ":\n";
	for(std::size_t i=0; i<coll.NumElements(); ++i)
		ostr << *coll.GetElement(i)<< "\n";

	return ostr;
}



// ----------------------------------------------------------------------------



Collections::Collections(const CollectionPtr coll)
	: m_cache{}, m_collections{}, m_transitions{}
{
	m_cache.insert(std::make_pair(coll->hash(), coll));
	m_collections.push_back(coll);
}


/**
 * perform all possible transitions from all collections
 * @return [source collection id, transition symbol, destination collection]
 */
void Collections::DoTransitions(const CollectionPtr coll_from)
{
	std::vector<std::tuple<SymbolPtr, CollectionPtr>> colls = coll_from->DoTransitions();

	// no more transitions?
	if(colls.size() == 0)
		return;

	for(const auto& tup : colls)
	{
		const SymbolPtr trans_sym = std::get<0>(tup);
		const CollectionPtr coll_to = std::get<1>(tup);
		std::size_t hash_to = coll_to->hash();

		auto cacheIter = m_cache.find(hash_to);
		if(cacheIter == m_cache.end())
		{
			// new unique collection
			m_cache.insert(std::make_pair(hash_to, coll_to));
			m_collections.push_back(coll_to);
			m_transitions.push_back(std::make_tuple(coll_from, coll_to, trans_sym));

			DoTransitions(coll_to);
		}
		else
		{
			// collection already seen
			m_transitions.push_back(std::make_tuple(coll_from, cacheIter->second, trans_sym));
		}
	}
}


void Collections::DoTransitions()
{
	DoTransitions(m_collections[0]);

	// cleanups
	std::sort(m_collections.begin(), m_collections.end(),
	[](const CollectionPtr coll1, const CollectionPtr coll2) -> bool
	{
		return coll1->GetId() < coll2->GetId();
	});

	std::stable_sort(m_transitions.begin(), m_transitions.end(),
	[](const auto& tup1, const auto& tup2) -> bool
	{
		CollectionPtr from1 = std::get<0>(tup1);
		CollectionPtr from2 = std::get<0>(tup2);
		CollectionPtr to1 = std::get<1>(tup1);
		CollectionPtr to2 = std::get<1>(tup2);

		if(from1->GetId() < from2->GetId())
			return true;
		if(from1->GetId() == from2->GetId())
			return to1->GetId() < to2->GetId();
		return false;
	});
}


std::ostream& operator<<(std::ostream& ostr, const Collections& colls)
{
	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Collections\n";
	ostr << "--------------------------------------------------------------------------------\n";
	for(const CollectionPtr& coll : colls.m_collections)
		ostr << *coll << "\n";

	ostr << "\n";

	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Transitions\n";
	ostr << "--------------------------------------------------------------------------------\n";
	for(const auto& tup : colls.m_transitions)
	{
		ostr << std::get<0>(tup)->GetId() << " -> " << std::get<1>(tup)->GetId()
			<< " via " << std::get<2>(tup)->GetId() << "\n";
	}

	return ostr;
}
