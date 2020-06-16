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

#include <fstream>
#include <sstream>
#include <algorithm>

#include <boost/functional/hash.hpp>


Element::Element(const NonTerminalPtr lhs, std::size_t rhsidx, std::size_t cursor, const Terminal::t_terminalset& la)
	: m_lhs{lhs}, m_rhs{&lhs->GetRule(rhsidx)}, m_semanticrule{lhs->GetSemanticRule(rhsidx)},
		m_rhsidx{rhsidx}, m_cursor{cursor}, m_lookaheads{la}
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
	return (*m_rhs)[m_cursor];
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



// ----------------------------------------------------------------------------



// global Closure id counter
std::size_t Closure::g_id = 0;


Closure::Closure() : m_elems{}, m_id{g_id++}
{
}


Closure::Closure(const Closure& coll) : m_elems{}
{
	this->operator=(coll);
}


const Closure& Closure::operator=(const Closure& coll)
{
	this->m_id = coll.m_id;
	for(ElementPtr elem : coll.m_elems)
		this->m_elems.emplace_back(std::make_shared<Element>(*elem));

	return *this;
}



/**
 * adds an element and generates the rest of the Closure
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

				std::map<std::string, Terminal::t_terminalset> tmp_first;
				calc_first(tmpNT, tmp_first);

				Terminal::t_terminalset first_la{Terminal::terminals_compare};
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
 * checks if an element is already in the Closure and returns its index
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
 * perform a transition and get the corresponding lr(1) Closure
 */
ClosurePtr Closure::DoTransition(const SymbolPtr transsym) const
{
	ClosurePtr newcoll = std::make_shared<Closure>();

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
 * perform all possible transitions from this Closure and get the corresponding lr(1) Collection
 * @return [transition symbol, destination Closure]
 */
std::vector<std::tuple<SymbolPtr, ClosurePtr>> Closure::DoTransitions() const
{
	std::vector<std::tuple<SymbolPtr, ClosurePtr>> colls;
	std::vector<SymbolPtr> possible_transitions = GetPossibleTransitions();

	for(const SymbolPtr& transition : possible_transitions)
	{
		ClosurePtr coll = DoTransition(transition);
		colls.emplace_back(std::make_tuple(transition, coll));
	}

	return colls;
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


std::ostream& operator<<(std::ostream& ostr, const Closure& coll)
{
	ostr << "Closure/State " << coll.GetId() << ":\n";
	for(std::size_t i=0; i<coll.NumElements(); ++i)
		ostr << *coll.GetElement(i)<< "\n";

	return ostr;
}



// ----------------------------------------------------------------------------



Collection::Collection(const ClosurePtr coll) : m_cache{}, m_collection{}, m_transitions{}
{
	m_cache.insert(std::make_pair(coll->hash(), coll));
	m_collection.push_back(coll);
}


Collection::Collection() : m_cache{}, m_collection{}, m_transitions{}
{
}


/**
 * perform all possible transitions from all Collection
 * @return [source Closure id, transition symbol, destination Closure]
 */
void Collection::DoTransitions(const ClosurePtr coll_from)
{
	std::vector<std::tuple<SymbolPtr, ClosurePtr>> colls = coll_from->DoTransitions();

	// no more transitions?
	if(colls.size() == 0)
		return;

	for(const auto& tup : colls)
	{
		const SymbolPtr trans_sym = std::get<0>(tup);
		const ClosurePtr coll_to = std::get<1>(tup);
		std::size_t hash_to = coll_to->hash();

		auto cacheIter = m_cache.find(hash_to);
		bool coll_to_new = (cacheIter == m_cache.end());

		//std::cout << "transition " << coll_from->GetId() << " -> " << coll_to->GetId()
		//	<< " via " << trans_sym->GetId() << ", new: " << coll_to_new << std::endl;
		//std::cout << std::hex << coll_to->hash() << ", " << *coll_to << std::endl;

		if(coll_to_new)
		{
			// new unique Closure
			m_cache.insert(std::make_pair(hash_to, coll_to));
			m_collection.push_back(coll_to);
			m_transitions.push_back(std::make_tuple(coll_from, coll_to, trans_sym));

			DoTransitions(coll_to);
		}
		else
		{
			// Closure already seen
			m_transitions.push_back(std::make_tuple(coll_from, cacheIter->second, trans_sym));
		}
	}
}


void Collection::DoTransitions()
{
	DoTransitions(m_collection[0]);
	Simplify();
}


void Collection::Simplify()
{
	constexpr bool do_sort = 1;
	constexpr bool do_cleanup = 1;

	// sort rules
	if constexpr(do_sort)
	{
		std::sort(m_collection.begin(), m_collection.end(),
		[](const ClosurePtr coll1, const ClosurePtr coll2) -> bool
		{
			return coll1->GetId() < coll2->GetId();
		});

		std::stable_sort(m_transitions.begin(), m_transitions.end(),
		[](const t_transition& tup1, const t_transition& tup2) -> bool
		{
			ClosurePtr from1 = std::get<0>(tup1);
			ClosurePtr from2 = std::get<0>(tup2);
			ClosurePtr to1 = std::get<1>(tup1);
			ClosurePtr to2 = std::get<1>(tup2);

			if(from1->GetId() < from2->GetId())
				return true;
			if(from1->GetId() == from2->GetId())
				return to1->GetId() < to2->GetId();
			return false;
		});
	}

	// cleanup ids
	if constexpr(do_cleanup)
	{
		std::map<std::size_t, std::size_t> idmap;
		std::set<std::size_t> already_seen;
		std::size_t newid = 0;

		for(ClosurePtr coll : m_collection)
		{
			std::size_t oldid = coll->GetId();
			std::size_t hash = coll->hash();

			if(already_seen.find(hash) != already_seen.end())
				continue;

			auto iditer = idmap.find(oldid);
			if(iditer == idmap.end())
				iditer = idmap.insert(std::make_pair(oldid, newid++)).first;

			coll->SetId(iditer->second);
			already_seen.insert(hash);
		}
	}
}


/**
 * write out the transitions graph
 */
void Collection::WriteGraph(const std::string& file, bool write_full_coll) const
{
	std::string outfile_graph = file + ".graph";
	std::string outfile_svg = file + ".svg";

	std::ofstream ofstr{outfile_graph};
	if(!ofstr)
		return;

	ofstr << "digraph G_lr1\n{\n";

	// write states
	for(const ClosurePtr& coll : m_collection)
	{
		ofstr << "\t" << coll->GetId() << " [label=\"";
		if(write_full_coll)
			ofstr << *coll;
		else
			ofstr << coll->GetId();
		ofstr << "\"];\n";
	}

	// write transitions
	ofstr << "\n";
	for(const t_transition& tup : m_transitions)
	{
		const ClosurePtr coll_from = std::get<0>(tup);
		const ClosurePtr coll_to = std::get<1>(tup);
		const SymbolPtr trans = std::get<2>(tup);

		ofstr << "\t" << coll_from->GetId() << " -> " << coll_to->GetId()
			<< " [label=\"" << trans->GetStrId() << "\"];\n";
	}

	ofstr << "}" << std::endl;
	ofstr.flush();
	ofstr.close();

	std::system(("dot -Tsvg " + outfile_graph + " -o " + outfile_svg).c_str());
}


/**
 * hash a transition element
 */
std::size_t Collection::hash_transition(const Collection::t_transition& trans)
{
	std::size_t hashFrom = std::get<0>(trans)->hash();
	std::size_t hashTo = std::get<1>(trans)->hash();
	std::size_t hashSym = std::get<2>(trans)->hash();

	boost::hash_combine(hashFrom, hashTo);
	boost::hash_combine(hashFrom, hashSym);

	return hashFrom;
}


/**
 * convert from LR(1) collection to LALR(1) collection
 */
Collection Collection::ConvertToLALR() const
{
	Collection colls;

	// maps old closure pointer to new one
	std::map<ClosurePtr, ClosurePtr> map;

	// states
	for(const ClosurePtr& coll : m_collection)
	{
		std::size_t hash = coll->hash(true);
		auto iter = colls.m_cache.find(hash);

		// closure core not yet seen
		if(iter == colls.m_cache.end())
		{
			ClosurePtr newcoll = std::make_shared<Closure>(*coll);
			map.insert(std::make_pair(coll, newcoll));

			colls.m_cache.insert(std::make_pair(hash, newcoll));
			colls.m_collection.push_back(newcoll);
		}

		// closure core already seen
		else
		{
			ClosurePtr collOld = iter->second;
			map.insert(std::make_pair(coll, collOld));

			// unite lookaheads
			for(std::size_t elemidx=0; elemidx<collOld->m_elems.size(); ++elemidx)
				collOld->m_elems[elemidx]->AddLookaheads(coll->m_elems[elemidx]->GetLookaheads());
		}
	}

	// transitions
	std::set<std::size_t> hashes;
	for(const t_transition& tup : m_transitions)
	{
		ClosurePtr collFromConv = map[std::get<0>(tup)];
		ClosurePtr collToConv = map[std::get<1>(tup)];

		t_transition trans = std::make_tuple(collFromConv, collToConv, std::get<2>(tup));

		std::size_t hash = hash_transition(trans);

		// transition already seen?
		if(hashes.find(hash) != hashes.end())
			continue;

		colls.m_transitions.emplace_back(std::move(trans));
		hashes.insert(hash);
	}

	colls.Simplify();
	return colls;
}


/**
 * convert from LR(1) collection to SLR(1) collection
 */
Collection Collection::ConvertToSLR(const std::map<std::string, Terminal::t_terminalset>& follow) const
{
	// reduce number of states first
	Collection colls = ConvertToLALR();

	// replace the lookaheads of all LR elements with the follow sets of their lhs symbols
	for(ClosurePtr& coll : colls.m_collection)
	{
		for(ElementPtr& elem : coll->m_elems)
		{
			const NonTerminalPtr& lhs = elem->GetLhs();
			const auto& iter = follow.find(lhs->GetStrId());
			if(iter == follow.end())
				throw std::runtime_error{"Could not find follow set of \"" + lhs->GetStrId() + "\"."};
			const Terminal::t_terminalset& followLhs = iter->second;
			elem->SetLookaheads(followLhs);
		}
	}

	return colls;
}


/**
 * creates the lr(1) parser tables
 */
std::tuple<t_table, t_table, t_table, t_mapIdIdx, t_mapIdIdx, t_vecIdx>
Collection::CreateParseTables() const
{
	const std::size_t numStates = m_collection.size();
	const std::size_t errorVal = ERROR_VAL;
	const std::size_t acceptVal = ACCEPT_VAL;

	// number of symbols on rhs of a production rule
	std::vector<std::size_t> numRhsSymsPerRule;

	// lr tables
	std::vector<std::vector<std::size_t>> _action_shift, _action_reduce, _jump;
	_action_shift.resize(numStates);
	_action_reduce.resize(numStates);
	_jump.resize(numStates);

	// maps the ids to table indices for the non-terminals
	t_mapIdIdx mapNonTermIdx;
	// maps the ids to table indices for the terminals
	t_mapIdIdx mapTermIdx;

	// current counters
	std::size_t curNonTermIdx = 0;
	std::size_t curTermIdx = 0;


	// translate symbol id to table index
	auto get_idx = [&mapNonTermIdx, &mapTermIdx, &curNonTermIdx, &curTermIdx]
	(std::size_t id, bool is_term) -> std::size_t
	{
		std::size_t* curIdx = is_term ? &curTermIdx : &curNonTermIdx;
		t_mapIdIdx* map = is_term ? &mapTermIdx : &mapNonTermIdx;

		auto iter = map->find(id);
		if(iter == map->end())
			iter = map->insert(std::make_pair(id, (*curIdx)++)).first;
		return iter->second;
	};


	for(const Collection::t_transition& tup : m_transitions)
	{
		const ClosurePtr& stateFrom = std::get<0>(tup);
		const ClosurePtr& stateTo = std::get<1>(tup);
		const SymbolPtr& symTrans = std::get<2>(tup);

		if(symTrans->IsTerminal())
		{
			std::size_t symIdx = get_idx(symTrans->GetId(), true);

			auto& _action_row = _action_shift[stateFrom->GetId()];
			if(_action_row.size() <= symIdx)
				_action_row.resize(symIdx+1, errorVal);
			_action_row[symIdx] = stateTo->GetId();
		}
		else
		{
			std::size_t symIdx = get_idx(symTrans->GetId(), false);

			auto& _jump_row = _jump[stateFrom->GetId()];
			if(_jump_row.size() <= symIdx)
				_jump_row.resize(symIdx+1, errorVal);
			_jump_row[symIdx] = stateTo->GetId();
		}
	}

	for(const ClosurePtr& coll : m_collection)
	{
		for(std::size_t elemidx=0; elemidx < coll->NumElements(); ++elemidx)
		{
			const ElementPtr& elem = coll->GetElement(elemidx);
			if(!elem->IsCursorAtEnd())
				continue;

			std::optional<std::size_t> rulenr = *elem->GetSemanticRule();
			if(!rulenr)		// no semantic rule assigned
				continue;
			std::size_t rule = *rulenr;

			const Word* rhs = elem->GetRhs();
			std::size_t numRhsSyms = rhs->NumSymbols();
			if(numRhsSymsPerRule.size() <= rule)
				numRhsSymsPerRule.resize(rule+1);
			numRhsSymsPerRule[rule] = numRhsSyms;

			auto& _action_row = _action_reduce[coll->GetId()];

			for(const auto& la : elem->GetLookaheads())
			{
				std::size_t laIdx = get_idx(la->GetId(), true);

				// in extended grammar, first production (rule 0) is of the form start -> ...
				if(rule == 0)
					rule = acceptVal;

				if(_action_row.size() <= laIdx)
					_action_row.resize(laIdx+1, errorVal);
				_action_row[laIdx] = rule;
			}
		}
	}


	const auto tables = std::make_tuple(
		t_table{_action_shift, errorVal, acceptVal, numStates, curTermIdx},
		t_table{_action_reduce, errorVal, acceptVal, numStates, curTermIdx},
		t_table{_jump, errorVal, acceptVal, numStates, curNonTermIdx},
		mapTermIdx, mapNonTermIdx,
		numRhsSymsPerRule);

	// check for shift/reduce conflicts
	for(std::size_t state=0; state<numStates; ++state)
	{
		for(std::size_t termidx=0; termidx<curTermIdx; ++termidx)
		{
			std::size_t shiftEntry = std::get<0>(tables)(state, termidx);
			std::size_t reduceEntry = std::get<1>(tables)(state, termidx);

			// both have an entry?
			if(shiftEntry!=errorVal && reduceEntry!=errorVal)
				throw std::runtime_error("Shift/reduce conflict detected.");
		}
	}

	return tables;
}


std::ostream& operator<<(std::ostream& ostr, const Collection& colls)
{
	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Collection\n";
	ostr << "--------------------------------------------------------------------------------\n";
	for(const ClosurePtr& coll : colls.m_collection)
		ostr << *coll << "\n";
	ostr << "\n";


	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Transitions\n";
	ostr << "--------------------------------------------------------------------------------\n";
	for(const Collection::t_transition& tup : colls.m_transitions)
	{
		ostr << std::get<0>(tup)->GetId() << " -> " << std::get<1>(tup)->GetId()
			<< " via " << std::get<2>(tup)->GetStrId() << "\n";
	}
	ostr << "\n\n";


	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Tables\n";
	ostr << "--------------------------------------------------------------------------------\n";
	std::ostringstream ostrActionShift, ostrActionReduce, ostrJump;
	for(const Collection::t_transition& tup : colls.m_transitions)
	{
		const ClosurePtr& stateFrom = std::get<0>(tup);
		const ClosurePtr& stateTo = std::get<1>(tup);
		const SymbolPtr& symTrans = std::get<2>(tup);

		if(symTrans->IsTerminal())
		{
			ostrActionShift << "action_shift[ state " << stateFrom->GetId() << ", "
				<< symTrans->GetStrId() << " ] = state " << stateTo->GetId() << "\n";
		}
		else
		{
			ostrJump << "jump[ state " << stateFrom->GetId() << ", "
				<< symTrans->GetStrId() << " ] = state " << stateTo->GetId() << "\n";
		}
	}

	for(const ClosurePtr& coll : colls.m_collection)
	{
		for(std::size_t elemidx=0; elemidx < coll->NumElements(); ++elemidx)
		{
			const ElementPtr& elem = coll->GetElement(elemidx);
			if(!elem->IsCursorAtEnd())
				continue;

			ostrActionReduce << "action_reduce[ state " << coll->GetId() << ", ";
			for(const auto& la : elem->GetLookaheads())
				ostrActionReduce << la->GetStrId() << " ";
			ostrActionReduce << "] = ";
			if(elem->GetSemanticRule())
				ostrActionReduce << "[rule " << *elem->GetSemanticRule() << "] ";
			ostrActionReduce << elem->GetLhs()->GetStrId() << " -> " << *elem->GetRhs();
			ostrActionReduce << "\n";
		}
	}

	ostr << ostrActionShift.str() << "\n" << ostrActionReduce.str() << "\n" << ostrJump.str() << "\n";
	return ostr;
}
