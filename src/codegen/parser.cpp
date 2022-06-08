/**
 * lr(1) parser
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 15-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 *	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 *	- "Übersetzerbau", ISBN: 978-3540653899 (1999, 2013)
 */

#include "parser.h"

#include <stack>


Parser::Parser(
	const std::tuple<
		t_table,			// shift table
		t_table,			// reduce table
		t_table,			// jump table
		t_mapIdIdx, 		// terminal indices
		t_mapIdIdx,			// nonterminal indices
		t_vecIdx>& init,	// semantic rule indices
	const std::vector<t_semanticrule>& rules)
	: m_tabActionShift{std::get<0>(init)},
		m_tabActionReduce{std::get<1>(init)},
		m_tabJump{std::get<2>(init)},
		m_mapTermIdx{std::get<3>(init)},
		m_mapNonTermIdx{std::get<4>(init)},
		m_numRhsSymsPerRule{std::get<5>(init)},
		m_semantics{rules}
{}


Parser::Parser(
	const std::tuple<
		const t_table*,		// shift table
		const t_table*,		// reduce table
		const t_table*,		// jump table
		const t_mapIdIdx*,	// terminal indices
		const t_mapIdIdx*,	// nonterminal indices
		const t_vecIdx*>& init,	// semantic rule indices
	const std::vector<t_semanticrule>& rules)
	: m_tabActionShift{*std::get<0>(init)},
		m_tabActionReduce{*std::get<1>(init)},
		m_tabJump{*std::get<2>(init)},
		m_mapTermIdx{*std::get<3>(init)},
		m_mapNonTermIdx{*std::get<4>(init)},
		m_numRhsSymsPerRule{*std::get<5>(init)},
		m_semantics{rules}
{}


t_astbaseptr Parser::Parse(const std::vector<t_toknode>& input) const
{
	constexpr bool debug = 0;

	std::stack<std::size_t> states;
	std::stack<t_astbaseptr> symbols;

	// starting state
	states.push(0);
	std::size_t inputidx = 0;

	t_toknode curtok = input[inputidx++];
	std::size_t curtokidx = curtok->GetTableIdx();

	while(true)
	{
		std::size_t topstate = states.top();
		std::size_t newstate = m_tabActionShift(topstate, curtokidx);
		std::size_t newrule = m_tabActionReduce(topstate, curtokidx);

		if(newstate == ERROR_VAL && newrule == ERROR_VAL)
		{
			throw std::runtime_error("Undefined shift and reduce entries.");
		}
		else if(newstate != ERROR_VAL && newrule != ERROR_VAL)
		{
			std::ostringstream ostrErr;
			ostrErr << "Shift/reduce conflict between shift"
				<< " from state " << topstate << " to state " << newstate
				<< " and reduce using rule " << newrule << ".";
			ostrErr << " Current token id is " << curtok->GetId()
				<< "." << std::endl;
			throw std::runtime_error(ostrErr.str());
		}

		// accept
		else if(newrule == ACCEPT_VAL)
		{
			if constexpr(debug)
				std::cout << "accepting" << std::endl;
			return symbols.top();
		}

		// shift
		else if(newstate != ERROR_VAL)
		{
			if constexpr(debug)
				std::cout << "shifting state " << newstate << std::endl;

			states.push(newstate);
			symbols.push(curtok);

			if(inputidx >= input.size())
				throw std::runtime_error("Input buffer underflow.");

			curtok = input[inputidx++];
			curtokidx = curtok->GetTableIdx();
		}

		// reduce
		else if(newrule != ERROR_VAL)
		{
			std::size_t numSyms = m_numRhsSymsPerRule[newrule];
			if constexpr(debug)
				std::cout << "reducing " << numSyms << " symbol(s) via rule " << newrule << std::endl;

			// take the symbols from the stack and create an argument vector for the semantic rule
			std::vector<t_astbaseptr> args;
			for(std::size_t arg=0; arg<numSyms; ++arg)
			{
				args.push_back(symbols.top());

				symbols.pop();
				states.pop();
			}

			if(args.size() > 1)
				std::reverse(args.begin(), args.end());

			// execute semantic rule
			t_astbaseptr reducedSym = m_semantics[newrule](args);
			symbols.push(reducedSym);

			topstate = states.top();
			std::size_t jumpstate = m_tabJump(topstate, reducedSym->GetTableIdx());
			states.push(jumpstate);
		}
	}

	return nullptr;
}
