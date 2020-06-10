/**
 * symbols
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#include "symbol.h"

#include <functional>
#include <boost/functional/hash.hpp>


TerminalPtr g_eps = std::make_shared<Terminal>("eps", true, false);
TerminalPtr g_end = std::make_shared<Terminal>("end", false, true);


// ----------------------------------------------------------------------------


bool Symbol::operator==(const Symbol& other) const
{
	return this->hash() == other.hash();
}


// ----------------------------------------------------------------------------



void Terminal::print(std::ostream& ostr) const
{
	ostr << GetId();
}


std::size_t Terminal::hash() const
{
	std::size_t hashId = std::hash<std::string>{}(GetId());
	std::size_t hashEps = std::hash<bool>{}(IsEps());
	std::size_t hashEnd = std::hash<bool>{}(IsEnd());

	boost::hash_combine(hashId, hashEps);
	boost::hash_combine(hashId, hashEnd);

	return hashId;
}


// ----------------------------------------------------------------------------


std::size_t NonTerminal::hash() const
{
	std::size_t hashId = std::hash<std::string>{}(GetId());
	return hashId;
}


void NonTerminal::print(std::ostream& ostr) const
{
	ostr << GetId() << " ->\n";
	for(std::size_t i=0; i<NumRules(); ++i)
	{
		if(i==0)
			ostr << "\t  ";
		else
			ostr << "\t| ";

		ostr << GetRule(i) << "\n";
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
		ostr << word.GetSymbol(i)->GetId() << " ";
	}

	return ostr;
}


// ----------------------------------------------------------------------------


/**
 * calculates the first set
 * see: https://www.cs.uaf.edu/~cs331/notes/FirstFollow.pdf
 */
void calc_first(const NonTerminalPtr nonterm,
	std::map<std::string, std::set<TerminalPtr>>& _first,
	std::map<std::string, std::vector<std::set<TerminalPtr>>>* _first_perrule)
{
	// set already calculated?
	if(_first.find(nonterm->GetId()) != _first.end())
		return;

	std::set<TerminalPtr> first;
	std::vector<std::set<TerminalPtr>> first_perrule;
	first_perrule.resize(nonterm->NumRules());

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
				for(const auto& symprod : _first[symnonterm->GetId()])
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

	_first[nonterm->GetId()] = first;

	if(_first_perrule)
		(*_first_perrule)[nonterm->GetId()] = first_perrule;
}


/**
 * calculates the follow set
 * see: https://www.cs.uaf.edu/~cs331/notes/FirstFollow.pdf
 */
void calc_follow(const std::vector<NonTerminalPtr>& allnonterms,
	const NonTerminalPtr& start, const NonTerminalPtr nonterm,
	std::map<std::string, std::set<SymbolPtr>>& _first,
	std::map<std::string, std::set<SymbolPtr>>& _follow)
{
	// set already calculated?
	if(_follow.find(nonterm->GetId()) != _follow.end())
		return;

	std::set<SymbolPtr> follow;


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
							follow.insert(rule[_iSym]);
							break;
						}
						else	// non-terminal
						{
							for(const auto& symfirst : _first[rule[_iSym]->GetId()])
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
						const auto& __follow = _follow[_nonterm->GetId()];
						follow.insert(__follow.begin(), __follow.end());
					}
				}
			}
		}
	}

	_follow[nonterm->GetId()] = follow;
}

