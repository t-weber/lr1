/**
 * symbols
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#include "symbol.h"


TerminalPtr g_eps = std::make_shared<Terminal>("eps", true, false);
TerminalPtr g_end = std::make_shared<Terminal>("end", false, true);


/**
 * calculates the first set
 * see: https://www.cs.uaf.edu/~cs331/notes/FirstFollow.pdf
 */
void calc_first(const NonTerminalPtr nonterm,
	std::map<std::string, std::set<SymbolPtr>>& _first,
	std::map<std::string, std::vector<std::set<SymbolPtr>>>* _first_perrule)
{
	// set already calculated?
	if(_first.find(nonterm->GetId()) != _first.end())
		return;

	std::set<SymbolPtr> first;
	std::vector<std::set<SymbolPtr>> first_perrule;
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
				first.insert(sym);
				first_perrule[iRule].insert(sym);
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
							first.insert(symprod);
							first_perrule[iRule].insert(symprod);
						}

						continue;
					}

					first.insert(symprod);
					first_perrule[iRule].insert(symprod);
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
