/**
 * lr(1) collection
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
#include <fstream>
#include <algorithm>

#include <boost/functional/hash.hpp>


Collection::Collection(const ClosurePtr closure)
	: m_cache{}, m_collection{}, m_transitions{}
{
	m_cache.insert(std::make_pair(closure->hash(), closure));
	m_collection.push_back(closure);
}


Collection::Collection()
	: m_cache{}, m_collection{}, m_transitions{}
{
}


/**
 * perform all possible transitions from all collection
 * @return [source closure id, transition symbol, destination closure]
 */
void Collection::DoTransitions(const ClosurePtr closure_from)
{
	std::vector<std::tuple<SymbolPtr, ClosurePtr>> coll =
		closure_from->DoTransitions();

	// no more transitions?
	if(coll.size() == 0)
		return;

	for(const auto& tup : coll)
	{
		const SymbolPtr trans_sym = std::get<0>(tup);
		const ClosurePtr closure_to = std::get<1>(tup);
		std::size_t hash_to = closure_to->hash();

		auto cacheIter = m_cache.find(hash_to);
		bool new_closure = (cacheIter == m_cache.end());

		//std::cout << "transition " << closure_from->GetId() << " -> " << closure_to->GetId()
		//	<< " via " << trans_sym->GetId() << ", new: " << new_closure << std::endl;
		//std::cout << std::hex << closure_to->hash() << ", " << *closure_to << std::endl;

		if(new_closure)
		{
			// new unique closure
			m_cache.insert(std::make_pair(hash_to, closure_to));
			m_collection.push_back(closure_to);
			m_transitions.push_back(std::make_tuple(
				closure_from, closure_to, trans_sym));

			DoTransitions(closure_to);
		}
		else
		{
			// reuse closure that has already been seen
			ClosurePtr closure_to_existing = cacheIter->second;

			m_transitions.push_back(std::make_tuple(
				closure_from, closure_to_existing, trans_sym));

			// also add the comefrom symbol to the closure
			closure_to_existing->AddComefromTransition(
				std::make_tuple(trans_sym, closure_from.get()));
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
		[](const ClosurePtr closure1, const ClosurePtr closure2) -> bool
		{
			return closure1->GetId() < closure2->GetId();
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

		for(ClosurePtr closure : m_collection)
		{
			std::size_t oldid = closure->GetId();
			std::size_t hash = closure->hash();

			if(already_seen.find(hash) != already_seen.end())
				continue;

			auto iditer = idmap.find(oldid);
			if(iditer == idmap.end())
				iditer = idmap.insert(std::make_pair(oldid, newid++)).first;

			closure->SetId(iditer->second);
			already_seen.insert(hash);
		}
	}
}


/**
 * write out the transitions graph
 */
bool Collection::WriteGraph(const std::string& file, bool write_full_coll) const
{
	std::string outfile_graph = file + ".graph";
	std::string outfile_svg = file + ".svg";

	std::ofstream ofstr{outfile_graph};
	if(!ofstr)
		return false;

	ofstr << "digraph G_lr1\n{\n";

	// write states
	for(const ClosurePtr& closure : m_collection)
	{
		ofstr << "\t" << closure->GetId() << " [label=\"";
		if(write_full_coll)
			ofstr << *closure;
		else
			ofstr << closure->GetId();
		ofstr << "\"];\n";
	}

	// write transitions
	ofstr << "\n";
	for(const t_transition& tup : m_transitions)
	{
		const ClosurePtr closure_from = std::get<0>(tup);
		const ClosurePtr closure_to = std::get<1>(tup);
		const SymbolPtr symTrans = std::get<2>(tup);

		//bool symIsTerm = symTrans->IsTerminal();
		bool symIsEps = symTrans->IsEps();

		if(symIsEps)
			continue;

		ofstr << "\t" << closure_from->GetId() << " -> " << closure_to->GetId()
			<< " [label=\"" << symTrans->GetStrId() << "\"];\n";
	}

	ofstr << "}" << std::endl;
	ofstr.flush();
	ofstr.close();

	std::system(("dot -Tsvg " + outfile_graph + " -o " + outfile_svg).c_str());
	return true;
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
	Collection coll;

	// maps old closure pointer to new one
	std::map<ClosurePtr, ClosurePtr> map;

	// states
	for(const ClosurePtr& closure : m_collection)
	{
		std::size_t hash = closure->hash(true);
		auto iter = coll.m_cache.find(hash);

		// closure core not yet seen
		if(iter == coll.m_cache.end())
		{
			ClosurePtr newclosure = std::make_shared<Closure>(*closure);
			map.insert(std::make_pair(closure, newclosure));

			coll.m_cache.insert(std::make_pair(hash, newclosure));
			coll.m_collection.push_back(newclosure);
		}

		// closure core already seen
		else
		{
			ClosurePtr closureOld = iter->second;
			map.insert(std::make_pair(closure, closureOld));

			// unite lookaheads
			for(std::size_t elemidx=0; elemidx<closureOld->m_elems.size(); ++elemidx)
				closureOld->m_elems[elemidx]->AddLookaheads(
					closure->m_elems[elemidx]->GetLookaheads());

			// unite lookbacks
			for(const Closure::t_comefrom_transition& comefrom
				: closure->m_comefrom_transitions)
				closureOld->AddComefromTransition(comefrom);
		}
	}

	// transitions
	std::set<std::size_t> hashes;
	for(const t_transition& tup : m_transitions)
	{
		ClosurePtr closureFromConv = map[std::get<0>(tup)];
		ClosurePtr closureToConv = map[std::get<1>(tup)];

		t_transition trans = std::make_tuple(
			closureFromConv, closureToConv, std::get<2>(tup));

		std::size_t hash = hash_transition(trans);

		// transition already seen?
		if(hashes.find(hash) != hashes.end())
			continue;

		coll.m_transitions.emplace_back(std::move(trans));
		hashes.insert(hash);
	}

	// update comefroms
	for(ClosurePtr& closure : coll.m_collection)
	{
		for(Closure::t_comefrom_transition& comefrom
			: closure->m_comefrom_transitions)
		{
			const Closure*& closure_from = std::get<1>(comefrom);
			// find closure_from in map (TODO: use ClosurePtr and map.find())
			for(const auto& [oldClosure, newClosure] : map)
			{
				if(oldClosure->GetId() == closure_from->GetId())
					closure_from = newClosure.get();
			}
		}

		closure->CleanComefromTransitions();
	}

	coll.Simplify();
	return coll;
}


/**
 * convert from LR(1) collection to SLR(1) collection
 */
Collection Collection::ConvertToSLR(const t_map_follow& follow) const
{
	// reduce number of states first
	Collection coll = ConvertToLALR();

	// replace the lookaheads of all LR elements with the follow sets of their lhs symbols
	for(ClosurePtr& closure : coll.m_collection)
	{
		for(ElementPtr& elem : closure->m_elems)
		{
			const NonTerminalPtr& lhs = elem->GetLhs();
			const auto& iter = follow.find(lhs);
			if(iter == follow.end())
			{
				throw std::runtime_error{
					"Could not find follow set of \"" +
					lhs->GetStrId() + "\"."};
			}

			const Terminal::t_terminalset& followLhs = iter->second;
			elem->SetLookaheads(followLhs);
		}
	}

	return coll;
}


/**
 * creates the lr(1) parser tables
 */
std::tuple<t_table, t_table, t_table, t_mapIdIdx, t_mapIdIdx, t_vecIdx>
Collection::CreateParseTables(
	const std::vector<t_conflictsolution>* conflictsol,
	bool stopOnConflicts) const
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

	// maps the ids to table indices for the non-terminals and the terminals
	t_mapIdIdx mapNonTermIdx, mapTermIdx;

	// current counters
	std::size_t curNonTermIdx = 0, curTermIdx = 0;

	// map terminal table index to terminal symbol
	std::unordered_map<std::size_t, TerminalPtr> seen_terminals{};


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

		bool symIsTerm = symTrans->IsTerminal();
		bool symIsEps = symTrans->IsEps();

		if(symIsEps)
			continue;

		std::vector<std::vector<std::size_t>>* tab =
			symIsTerm ? &_action_shift : &_jump;

		std::size_t symIdx = get_idx(symTrans->GetId(), symIsTerm);
		if(symIsTerm)
		{
			seen_terminals.insert(std::make_pair(
				symIdx, std::dynamic_pointer_cast<Terminal>(symTrans)));
		}

		auto& _tab_row = (*tab)[stateFrom->GetId()];
		if(_tab_row.size() <= symIdx)
			_tab_row.resize(symIdx+1, errorVal);
		_tab_row[symIdx] = stateTo->GetId();
	}

	for(const ClosurePtr& closure : m_collection)
	{
		for(std::size_t elemidx=0; elemidx < closure->NumElements(); ++elemidx)
		{
			const ElementPtr& elem = closure->GetElement(elemidx);
			if(!elem->IsCursorAtEnd())
				continue;

			std::optional<std::size_t> rulenr = *elem->GetSemanticRule();
			if(!rulenr)		// no semantic rule assigned
				continue;
			std::size_t rule = *rulenr;

			const Word* rhs = elem->GetRhs();
			std::size_t numRhsSyms = rhs->NumSymbols(false);
			if(numRhsSymsPerRule.size() <= rule)
				numRhsSymsPerRule.resize(rule+1);
			numRhsSymsPerRule[rule] = numRhsSyms;

			auto& _action_row = _action_reduce[closure->GetId()];

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


	auto tables = std::make_tuple(
		t_table{_action_shift, errorVal, acceptVal, numStates, curTermIdx},
		t_table{_action_reduce, errorVal, acceptVal, numStates, curTermIdx},
		t_table{_jump, errorVal, acceptVal, numStates, curNonTermIdx},
		mapTermIdx, mapNonTermIdx,
		numRhsSymsPerRule);

	// check for and try to resolve shift/reduce conflicts
	for(std::size_t state=0; state<numStates; ++state)
	{
		ClosurePtr closureState = m_collection[state];
		std::vector<TerminalPtr> comefromTerms = closureState->GetComefromTerminals();

		for(std::size_t termidx=0; termidx<curTermIdx; ++termidx)
		{
			std::size_t& shiftEntry = std::get<0>(tables)(state, termidx);
			std::size_t& reduceEntry = std::get<1>(tables)(state, termidx);
			//const t_mapIdIdx& mapTermIdx = std::get<3>(tables);

			std::optional<std::string> termid;
			ElementPtr conflictelem = nullptr;
			if(auto termiter = seen_terminals.find(termidx);
				termiter != seen_terminals.end())
			{
				termid = termiter->second->GetStrId();
				conflictelem = closureState->
					GetElementWithCursorAtSymbol(termiter->second);
			}

			// both have an entry?
			if(shiftEntry!=errorVal && reduceEntry!=errorVal)
			{
				bool solution_found = false;

				// try to resolve conflict
				if(conflictsol && conflictelem)
				{
					for(const t_conflictsolution& sol : *conflictsol)
					{
						ConflictSolution solution = ConflictSolution::NONE;

						// solution has a lhs nonterminal and a lookahead terminal
						if(std::holds_alternative<NonTerminalPtr>(std::get<0>(sol)) &&
							*std::get<NonTerminalPtr>(std::get<0>(sol)) == *conflictelem->GetLhs() &&
							*std::get<1>(sol) == *conflictelem->GetSymbolAtCursor())
						{
							solution = std::get<2>(sol);
						}

						// solution has a lookback terminal and a lookahead terminal
						else if(std::holds_alternative<TerminalPtr>(std::get<0>(sol)) &&
							*std::get<1>(sol) == *conflictelem->GetSymbolAtCursor())
						{
							// see if the given lookback terminal is in the closure's list
							for(const TerminalPtr& comefromTerm : comefromTerms)
							{
								if(*comefromTerm == *std::get<TerminalPtr>(std::get<0>(sol)))
								{
									solution = std::get<2>(sol);
									break;
								}
							}
						}

						switch(solution)
						{
							case ConflictSolution::FORCE_SHIFT:
								reduceEntry = errorVal;
								solution_found = true;
								break;
							case ConflictSolution::FORCE_REDUCE:
								shiftEntry = errorVal;
								solution_found = true;
								break;
							case ConflictSolution::NONE:
								solution_found = false;
								break;
						}

						if(solution_found)
							break;
					}
				}

				if(!solution_found)
				{
					std::ostringstream ostrErr;
					ostrErr << "Shift/reduce conflict detected"
						<< " for state " << state;
					if(conflictelem)
						ostrErr << ":\n\t" << *conflictelem << "\n";
					if(comefromTerms.size())
					{
						ostrErr << " with look-back terminal(s): ";
						for(std::size_t i=0; i<comefromTerms.size(); ++i)
						{
							ostrErr << comefromTerms[i]->GetStrId();
							if(i < comefromTerms.size()-1)
								ostrErr << ", ";
						}
					}
					if(termid)
						ostrErr << " and look-ahead terminal " << (*termid);
					else
						ostrErr << "and terminal index " << termidx;
					ostrErr << " (can either shift to state " << shiftEntry
						<< " or reduce using rule " << reduceEntry
						<< ")." << std::endl;

					if(stopOnConflicts)
						throw std::runtime_error(ostrErr.str());
					else
						std::cerr << ostrErr.str() << std::endl;
				}
			}
		}
	}

	return tables;
}


/**
 * export lr(1) tables to C++ code
 */
bool Collection::SaveParseTables(const std::tuple<t_table, t_table, t_table,
	t_mapIdIdx, t_mapIdIdx, t_vecIdx>& tabs, const std::string& file)
{
	std::ofstream ofstr{file};
	if(!ofstr)
		return false;

	ofstr << "#ifndef __LR1_TABLES__\n";
	ofstr << "#define __LR1_TABLES__\n\n";

	ofstr <<"namespace _lr1_tables {\n\n";

	std::get<0>(tabs).SaveCXXDefinition(ofstr, "tab_action_shift");
	std::get<1>(tabs).SaveCXXDefinition(ofstr, "tab_action_reduce");
	std::get<2>(tabs).SaveCXXDefinition(ofstr, "tab_jump");

	ofstr << "const t_mapIdIdx map_term_idx{{\n";
	for(const auto& pair : std::get<3>(tabs))
		ofstr << "\t{" << pair.first << ", " << pair.second << "},\n";
	ofstr << "}};\n\n";

	ofstr << "const t_mapIdIdx map_nonterm_idx{{\n";
	for(const auto& pair : std::get<4>(tabs))
		ofstr << "\t{" << pair.first << ", " << pair.second << "},\n";
	ofstr << "}};\n\n";

	ofstr << "const t_vecIdx vec_num_rhs_syms{{ ";
	for(const auto& val : std::get<5>(tabs))
		ofstr << val << ",";
	ofstr << " }};\n\n";

	ofstr << "}\n\n\n";


	ofstr << "static std::tuple<const t_table*, const t_table*, const t_table*, const t_mapIdIdx*, const t_mapIdIdx*, const t_vecIdx*>\n";
	ofstr << "get_lr1_tables()\n{\n";
	ofstr << "\treturn std::make_tuple(\n"
		<< "\t\t&_lr1_tables::tab_action_shift, &_lr1_tables::tab_action_reduce, &_lr1_tables::tab_jump,\n"
		<< "\t\t&_lr1_tables::map_term_idx, &_lr1_tables::map_nonterm_idx, &_lr1_tables::vec_num_rhs_syms);\n";
	ofstr << "}\n\n";


	ofstr << "\n#endif" << std::endl;
	return true;
}


std::ostream& operator<<(std::ostream& ostr, const Collection& coll)
{
	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Collection\n";
	ostr << "--------------------------------------------------------------------------------\n";
	for(const ClosurePtr& closure : coll.m_collection)
		ostr << *closure << "\n";
	ostr << "\n";


	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Transitions\n";
	ostr << "--------------------------------------------------------------------------------\n";
	for(const Collection::t_transition& tup : coll.m_transitions)
	{
		ostr << std::get<0>(tup)->GetId() << " -> " << std::get<1>(tup)->GetId()
			<< " via " << std::get<2>(tup)->GetStrId() << "\n";
	}
	ostr << "\n\n";


	ostr << "--------------------------------------------------------------------------------\n";
	ostr << "Tables\n";
	ostr << "--------------------------------------------------------------------------------\n";
	std::ostringstream ostrActionShift, ostrActionReduce, ostrJump;
	for(const Collection::t_transition& tup : coll.m_transitions)
	{
		const ClosurePtr& stateFrom = std::get<0>(tup);
		const ClosurePtr& stateTo = std::get<1>(tup);
		const SymbolPtr& symTrans = std::get<2>(tup);

		bool symIsTerm = symTrans->IsTerminal();
		bool symIsEps = symTrans->IsEps();

		if(symIsEps)
			continue;

		if(symIsTerm)
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

	for(const ClosurePtr& closure : coll.m_collection)
	{
		for(std::size_t elemidx=0; elemidx < closure->NumElements(); ++elemidx)
		{
			const ElementPtr& elem = closure->GetElement(elemidx);
			if(!elem->IsCursorAtEnd())
				continue;

			ostrActionReduce << "action_reduce[ state " << closure->GetId() << ", ";
			for(const auto& la : elem->GetLookaheads())
				ostrActionReduce << la->GetStrId() << " ";
			ostrActionReduce << "] = ";
			if(elem->GetSemanticRule())
				ostrActionReduce << "[rule " << *elem->GetSemanticRule() << "] ";
			ostrActionReduce << elem->GetLhs()->GetStrId() << " -> " << *elem->GetRhs();
			ostrActionReduce << "\n";
		}
	}

	ostr << ostrActionShift.str() << "\n"
		<< ostrActionReduce.str() << "\n"
		<< ostrJump.str() << "\n";
	return ostr;
}
