/**
 * reduce/reduce and shift/reduce conflict tests
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 27-august-2022
 * @license see 'LICENSE.EUPL' file
 */
#include "parsergen/lr1.h"
#include "codegen/lexer.h"
#include "codegen/parser.h"

#include <iostream>
#include <sstream>
#include <iomanip>


/**
 * tests grammar that is LR(1), but not LALR(1) or SLR(1)
 */
void lr1_grammar()
{
	enum : std::size_t { PROD_SPRIME, PROD_S, PROD_A, PROD_B };

	auto Sprime = std::make_shared<NonTerminal>(PROD_SPRIME, "S'");
	auto S = std::make_shared<NonTerminal>(PROD_S, "S");
	auto A = std::make_shared<NonTerminal>(PROD_A, "A");
	auto B = std::make_shared<NonTerminal>(PROD_B, "B");

	auto a = std::make_shared<Terminal>('a', "a");
	auto b = std::make_shared<Terminal>('b', "b");


	// productions
	// see: https://web.stanford.edu/class/archive/cs/cs143/cs143.1128/handouts/140%20LALR%20Parsing.pdf, p. 3
	std::size_t semanticindex = 0;

	Sprime->AddRule({ S }, semanticindex++);

	S->AddRule({ a, A, a }, semanticindex++);
	S->AddRule({ a, B, b }, semanticindex++);
	S->AddRule({ b, B, a }, semanticindex++);
	S->AddRule({ b, A, b }, semanticindex++);

	A->AddRule({ a }, semanticindex++);
	B->AddRule({ a }, semanticindex++);

	std::vector<NonTerminalPtr> all_nonterminals{{ Sprime, S, A, B }};

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
	coll->AddElement(elem);

	std::cout << "\n\n================================================================================" << std::endl;
	std::cout << "Calculating LR(1) closures..." << std::endl;
	Collection colls{coll};
	colls.DoTransitions();
	colls.WriteGraph("reduce_conflict", 1);
	std::cout << "\n\nLR(1):\n" << colls << std::endl;
	std::cout << "================================================================================" << std::endl;

	std::cout << "\n\n================================================================================" << std::endl;
	std::cout << "Converting grammar to LALR(1)..." << std::endl;
	Collection collsLALR = colls.ConvertToLALR();
	collsLALR.WriteGraph("reduce_conflict_lalr", 1);
	std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;
	std::cout << "================================================================================" << std::endl;

	std::cout << "\n\n================================================================================" << std::endl;
	std::cout << "Converting grammar to SLR(1)..." << std::endl;
	Collection collsSLR = colls.ConvertToSLR(follow);
	collsSLR.WriteGraph("reduce_conflict_slr", 1);
	std::cout << "\n\nSLR(1):\n" << collsSLR << std::endl;
	std::cout << "================================================================================" << std::endl;
}


/**
 * tests grammar that is LALR(1), but not SLR(1)
 */
void lalr1_grammar()
{
	enum : std::size_t { PROD_SPRIME, PROD_S, PROD_A };

	auto Sprime = std::make_shared<NonTerminal>(PROD_SPRIME, "S'");
	auto S = std::make_shared<NonTerminal>(PROD_S, "S");
	auto A = std::make_shared<NonTerminal>(PROD_A, "A");

	auto a = std::make_shared<Terminal>('a', "a");
	auto b = std::make_shared<Terminal>('b', "b");


	// productions
	// see: https://web.stanford.edu/class/archive/cs/cs143/cs143.1128/handouts/140%20LALR%20Parsing.pdf, p. 2
	std::size_t semanticindex = 0;

	Sprime->AddRule({ S }, semanticindex++);

	S->AddRule({ A, a, a }, semanticindex++);
	S->AddRule({ a, A, b }, semanticindex++);
	S->AddRule({ b, b, a }, semanticindex++);

	A->AddRule({ b }, semanticindex++);

	std::vector<NonTerminalPtr> all_nonterminals{{ Sprime, S, A }};

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
	coll->AddElement(elem);

	std::cout << "\n\n================================================================================" << std::endl;
	std::cout << "Calculating LR(1) closures..." << std::endl;
	Collection colls{coll};
	colls.DoTransitions();
	colls.WriteGraph("reduce_conflict", 1);
	std::cout << "\n\nLR(1):\n" << colls << std::endl;
	std::cout << "================================================================================" << std::endl;

	std::cout << "\n\n================================================================================" << std::endl;
	std::cout << "Converting grammar to LALR(1)..." << std::endl;
	Collection collsLALR = colls.ConvertToLALR();
	collsLALR.WriteGraph("reduce_conflict_lalr", 1);
	std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;
	std::cout << "================================================================================" << std::endl;

	std::cout << "\n\n================================================================================" << std::endl;
	std::cout << "Converting grammar to SLR(1)..." << std::endl;
	Collection collsSLR = colls.ConvertToSLR(follow);
	collsSLR.WriteGraph("reduce_conflict_slr", 1);
	std::cout << "\n\nSLR(1):\n" << collsSLR << std::endl;
	std::cout << "================================================================================" << std::endl;
}


int main()
{
	std::cout << "********************************************************************************" << std::endl;
	std::cout << "LR(1) grammar test." << std::endl;
	std::cout << "********************************************************************************" << std::endl;
	lr1_grammar();

	std::cout << "\n\n\n\n********************************************************************************" << std::endl;
	std::cout << "LALR(1) grammar test." << std::endl;
	std::cout << "********************************************************************************" << std::endl;
	lalr1_grammar();
	return 0;
}
