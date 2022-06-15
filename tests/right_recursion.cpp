/**
 * test
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 8-jun-2022
 * @license see 'LICENSE.EUPL' file
 */
#include "parsergen/lr1.h"
#include "codegen/lexer.h"
#include "codegen/parser.h"

#include <iostream>
#include <sstream>
#include <iomanip>

enum : std::size_t { PROD_SPRIME, PROD_S, PROD_AS, PROD_A, };

int main()
{
	auto Sprime = std::make_shared<NonTerminal>(PROD_SPRIME, "S'");
	auto S = std::make_shared<NonTerminal>(PROD_SPRIME, "S");
	auto As = std::make_shared<NonTerminal>(PROD_AS, "As");
	auto A = std::make_shared<NonTerminal>(PROD_A, "A");

	auto a = std::make_shared<Terminal>('a', "a");
	auto b = std::make_shared<Terminal>('b', "b");


	// productions
	std::size_t semanticindex = 0;

	Sprime->AddRule({ S }, semanticindex++);

	S->AddRule({ As }, semanticindex++);

	As->AddRule({ A, As }, semanticindex++);
	As->AddRule({ g_eps }, semanticindex++);

	A->AddRule({ a, b }, semanticindex++);

	std::vector<NonTerminalPtr> all_nonterminals{{ Sprime, S, As, A }};

	std::cout << "Productions:\n";
	for(NonTerminalPtr nonterm : all_nonterminals)
		nonterm->print(std::cout, true);
	std::cout << std::endl;


	std::cout << "FIRST sets:\n";
	t_map_first first;
	t_map_first_perrule first_per_rule;
	for(const NonTerminalPtr& nonterminal : all_nonterminals)
		calc_first(nonterminal, first, &first_per_rule);

	for(const auto& pair : first)
	{
		std::cout << pair.first->GetStrId() << ": ";
		for(const auto& _first : pair.second)
			std::cout << _first->GetStrId() << ", ";
		std::cout << "\n";
	}
	std::cout << std::endl;


	std::cout << "FOLLOW sets:\n";
	t_map_follow follow;
	for(const NonTerminalPtr& nonterminal : all_nonterminals)
		calc_follow(all_nonterminals, Sprime, nonterminal, first, follow);

	for(const auto& pair : follow)
	{
		std::cout << pair.first->GetStrId() << ": ";
		for(const auto& _first : pair.second)
			std::cout << _first->GetStrId() << ", ";
		std::cout << "\n";
	}
	std::cout << std::endl;


	ElementPtr elem = std::make_shared<Element>(
		Sprime, 0, 0, Terminal::t_terminalset{{ g_end }});
	ClosurePtr coll = std::make_shared<Closure>();
	coll->SetThisPtr(coll);
	coll->AddElement(elem);

	Collection colls{coll};
	colls.DoTransitions();
	colls.WriteGraph("right_recursion", 1);
	std::cout << "\n\nLR(1):\n" << colls << std::endl;

	Collection collsLALR = colls.ConvertToLALR();
	collsLALR.WriteGraph("right_recursion_lalr", 1);
	std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;

	return 0;
}
