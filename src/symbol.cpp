/**
 * symbols
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#include "symbol.h"

#include <boost/functional/hash.hpp>


const TerminalPtr g_eps = std::make_shared<Terminal>(0xffffff00, "eps", true, false);
const TerminalPtr g_end = std::make_shared<Terminal>(0xffffff01, "end", false, true);


// ----------------------------------------------------------------------------


Symbol::Symbol(std::size_t id, const std::string& strid, bool bEps, bool bEnd)
	: m_id{id}, m_strid{strid}, m_idx{}, m_iseps{bEps}, m_isend{bEnd}
{
	// if no string id is given, use the numeric id
	if(m_strid == "")
		m_strid = std::to_string(id);
}


bool Symbol::operator==(const Symbol& other) const
{
	return this->hash() == other.hash();
}


// ----------------------------------------------------------------------------



void Terminal::print(std::ostream& ostr) const
{
	ostr << GetStrId();
}


std::size_t Terminal::hash() const
{
	//std::size_t hashId = std::hash<std::string>{}(GetStrId());
	std::size_t hashId = std::hash<std::size_t>{}(GetId());
	std::size_t hashEps = std::hash<bool>{}(IsEps());
	std::size_t hashEnd = std::hash<bool>{}(IsEnd());

	boost::hash_combine(hashId, hashEps);
	boost::hash_combine(hashId, hashEnd);

	return hashId;
}


/**
 * comparator for set of terminals
 */
std::function<bool(const TerminalPtr term1, const TerminalPtr term2)>
Terminal::terminals_compare = [](const TerminalPtr term1, const TerminalPtr term2) -> bool
{
	/*
	const std::string& id1 = term1->GetStrId();
	const std::string& id2 = term2->GetStrId();
	return std::lexicographical_compare(id1.begin(), id1.end(), id2.begin(), id2.end());
	*/
	return term1->hash() < term2->hash();
};


// ----------------------------------------------------------------------------


std::size_t NonTerminal::hash() const
{
	//std::size_t hashId = std::hash<std::string>{}(GetStrId());
	std::size_t hashId = std::hash<std::size_t>{}(GetId());
	return hashId;
}


void NonTerminal::print(std::ostream& ostr) const
{
	ostr << GetStrId() << " ->\n";
	for(std::size_t i=0; i<NumRules(); ++i)
	{
		if(i==0)
			ostr << "\t  ";
		else
			ostr << "\t| ";

		if(GetSemanticRule(i))
			ostr << "[rule " << *GetSemanticRule(i) << "] ";

		ostr << GetRule(i);
		ostr << "\n";
	}
}


/**
 * does this non-terminal have a rule which produces epsilon?
 */
bool NonTerminal::HasEpsRule() const
{
	for(const auto& rule : m_rules)
	{
		if(rule.NumSymbols()==1 && rule[0]->IsEps())
			return true;
	}
	return false;
}


// ----------------------------------------------------------------------------


bool Word::operator==(const Word& other) const
{
	if(this->NumSymbols() != other.NumSymbols())
		return false;

	bool bMatch = true;
	for(std::size_t i=0; i<NumSymbols(); ++i)
	{
		if(*m_syms[i] != *other.m_syms[i])
		{
			bMatch = false;
			break;
		}
	}

	return bMatch;
}


std::size_t Word::hash() const
{
	std::size_t hash = 0;

	for(std::size_t i=0; i<NumSymbols(); ++i)
	{
		std::size_t hashSym = m_syms[i]->hash();
		boost::hash_combine(hash, hashSym);
	}

	return hash;
}


std::ostream& operator<<(std::ostream& ostr, const Word& word)
{
	for(std::size_t i=0; i<word.NumSymbols(); ++i)
	{
		//word.GetSymbol(i)->print(ostr);
		ostr << word.GetSymbol(i)->GetStrId() << " ";
	}

	return ostr;
}


// ----------------------------------------------------------------------------


/**
 * calculates the first set of a nonterminal
 * see: https://www.cs.uaf.edu/~cs331/notes/FirstFollow.pdf
 * TODO: use symbol ids, not string ids
 */
void calc_first(const NonTerminalPtr nonterm,
	std::map<std::string, Terminal::t_terminalset>& _first,
	std::map<std::string, std::vector<Terminal::t_terminalset>>* _first_perrule)
{
	// set already calculated?
	if(_first.find(nonterm->GetStrId()) != _first.end())
		return;

	Terminal::t_terminalset first{Terminal::terminals_compare};
	std::vector<Terminal::t_terminalset> first_perrule;

	// initialise first_perrule set, this is important because of the terminals_compare comparator
	first_perrule.reserve(nonterm->NumRules());
	for(std::size_t i=0; i<nonterm->NumRules(); ++i)
		first_perrule.emplace_back(Terminal::t_terminalset{Terminal::terminals_compare});

	// iterate rules
	for(std::size_t iRule=0; iRule<nonterm->NumRules(); ++iRule)
	{
		const auto& rule = nonterm->GetRule(iRule);

		// iterate RHS of rule
		for(std::size_t iSym=0; iSym<rule.NumSymbols(); ++iSym)
		{
			const auto& sym = rule[iSym];

			// reached terminal symbol -> end
			if(sym->IsTerminal())
			{
				first.insert(std::dynamic_pointer_cast<Terminal>(sym));
				first_perrule[iRule].insert(std::dynamic_pointer_cast<Terminal>(sym));
				break;
			}

			// non-terminal
			else
			{
				const NonTerminalPtr symnonterm = std::dynamic_pointer_cast<NonTerminal>(sym);

				// if the rule is left-recursive, ignore calculating the same symbol again
				if(*symnonterm != *nonterm)
					calc_first(symnonterm, _first, _first_perrule);

				// add first set except eps
				bool bHasEps = false;
				for(const auto& symprod : _first[symnonterm->GetStrId()])
				{
					if(symprod->IsEps())
					{
						bHasEps = true;

						// last non-terminal reached -> add epsilon
						if(iSym == rule.NumSymbols()-1)
						{
							first.insert(std::dynamic_pointer_cast<Terminal>(symprod));
							first_perrule[iRule].insert(std::dynamic_pointer_cast<Terminal>(symprod));
						}

						continue;
					}

					first.insert(std::dynamic_pointer_cast<Terminal>(symprod));
					first_perrule[iRule].insert(std::dynamic_pointer_cast<Terminal>(symprod));
				}

				// no epsilon in production -> end
				if(!bHasEps)
					break;
			}
		}
	}

	_first[nonterm->GetStrId()] = first;

	if(_first_perrule)
		(*_first_perrule)[nonterm->GetStrId()] = first_perrule;
}


/**
 * calculates the follow set of a nonterminal
 * see: https://www.cs.uaf.edu/~cs331/notes/FirstFollow.pdf
 * TODO: use symbol ids, not string ids
 */
void calc_follow(const std::vector<NonTerminalPtr>& allnonterms,
	const NonTerminalPtr& start, const NonTerminalPtr nonterm,
	const std::map<std::string, Terminal::t_terminalset>& _first,
	std::map<std::string, Terminal::t_terminalset>& _follow)
{
	// set already calculated?
	if(_follow.find(nonterm->GetStrId()) != _follow.end())
		return;

	Terminal::t_terminalset follow{Terminal::terminals_compare};


	// add end symbol as follower to start rule
	if(nonterm == start)
		follow.insert(g_end);


	// find current nonterminal in RHS of all rules (to get following symbols)
	for(const auto& _nonterm : allnonterms)
	{
		// iterate rules
		for(std::size_t iRule=0; iRule<_nonterm->NumRules(); ++iRule)
		{
			const auto& rule = _nonterm->GetRule(iRule);

			// iterate RHS of rule
			for(std::size_t iSym=0; iSym<rule.NumSymbols(); ++iSym)
			{
				// nonterm is in RHS of _nonterm rules
				if(*rule[iSym] == *nonterm)
				{
					// add first set of following symbols except eps
					for(std::size_t _iSym=iSym+1; _iSym < rule.NumSymbols(); ++_iSym)
					{
						// add terminal to follow set
						if(rule[_iSym]->IsTerminal() && !rule[_iSym]->IsEps())
						{
							follow.insert(std::dynamic_pointer_cast<Terminal>(rule[_iSym]));
							break;
						}
						else	// non-terminal
						{
							const auto& iterFirst = _first.find(rule[_iSym]->GetStrId());

							for(const auto& symfirst : iterFirst->second)
							{
								if(!symfirst->IsEps())
									follow.insert(symfirst);
							}

							const NonTerminalPtr symnonterm = std::dynamic_pointer_cast<NonTerminal>(rule[_iSym]);

							if(!symnonterm->HasEpsRule())
								break;
						}
					}


					// last symbol in rule?
					bool bLastSym = (iSym+1 == rule.NumSymbols());

					// ... or only epsilon productions afterwards?
					std::size_t iNextSym = iSym+1;
					for(; iNextSym<rule.NumSymbols(); ++iNextSym)
					{
						if(rule[iNextSym]->IsTerminal())
							break;

						const NonTerminalPtr symnonterm = std::dynamic_pointer_cast<NonTerminal>(rule[iNextSym]);

						if(!symnonterm->HasEpsRule())
							break;
					}

					if(bLastSym || iNextSym==rule.NumSymbols())
					{
						if(_nonterm != nonterm)
							calc_follow(allnonterms, start, _nonterm, _first, _follow);
						const auto& __follow = _follow[_nonterm->GetStrId()];
						follow.insert(__follow.begin(), __follow.end());
					}
				}
			}
		}
	}

	_follow[nonterm->GetStrId()] = follow;
}
