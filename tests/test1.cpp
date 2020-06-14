/**
 * test
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#include "lr1.h"
#include <iostream>


int main()
{
	bool bSimplifiedGrammar = 1;

	// test grammar from: https://de.wikipedia.org/wiki/LL(k)-Grammatik#Beispiel
	auto start = std::make_shared<NonTerminal>("start");
	auto add_term = std::make_shared<NonTerminal>("add_term");
	auto mul_term = std::make_shared<NonTerminal>("mul_term");
	auto pow_term = std::make_shared<NonTerminal>("pow_term");
	auto factor = std::make_shared<NonTerminal>("factor");

	auto plus = std::make_shared<Terminal>("+");
	auto minus = std::make_shared<Terminal>("-");
	auto mult = std::make_shared<Terminal>("*");
	auto div = std::make_shared<Terminal>("/");
	auto mod = std::make_shared<Terminal>("%");
	auto pow = std::make_shared<Terminal>("^");
	auto bracket_open = std::make_shared<Terminal>("(");
	auto bracket_close = std::make_shared<Terminal>(")");
	auto comma = std::make_shared<Terminal>(",");
	auto sym = std::make_shared<Terminal>("symbol");
	auto ident = std::make_shared<Terminal>("ident");

	std::size_t semanticindex = 0;
	start->AddRule({ add_term }, semanticindex++);

	add_term->AddRule({ add_term, plus, mul_term }, semanticindex++);
	if(!bSimplifiedGrammar)
		add_term->AddRule({ add_term, minus, mul_term }, semanticindex++);
	add_term->AddRule({ mul_term }, semanticindex++);

	if(bSimplifiedGrammar)
	{
		mul_term->AddRule({ mul_term, mult, factor }, semanticindex++);
	}
	else
	{
		mul_term->AddRule({ mul_term, mult, pow_term }, semanticindex++);
		mul_term->AddRule({ mul_term, div, pow_term }, semanticindex++);
		mul_term->AddRule({ mul_term, mod, pow_term }, semanticindex++);
	}

	if(bSimplifiedGrammar)
		mul_term->AddRule({ factor }, semanticindex++);
	else
		mul_term->AddRule({ pow_term }, semanticindex++);

	pow_term->AddRule({ pow_term, pow, factor }, semanticindex++);
	pow_term->AddRule({ factor }, semanticindex++);

	factor->AddRule({ bracket_open, add_term, bracket_close }, semanticindex++);
	if(!bSimplifiedGrammar)
	{
		factor->AddRule({ ident, bracket_open, bracket_close }, semanticindex++);			// function call
		factor->AddRule({ ident, bracket_open, add_term, bracket_close }, semanticindex++);			// function call
		factor->AddRule({ ident, bracket_open, add_term, comma, add_term, bracket_close }, semanticindex++);	// function call
	}
	factor->AddRule({ sym }, semanticindex++);


	std::vector<NonTerminalPtr> all_nonterminals{{start, add_term, mul_term, factor}};
	if(!bSimplifiedGrammar)
		all_nonterminals.push_back(pow_term);

	std::cout << "Productions:\n";
	for(NonTerminalPtr nonterm : all_nonterminals)
		nonterm->print(std::cout);
	std::cout << std::endl;


	std::cout << "FIRST sets:\n";
	std::map<std::string, Terminal::t_terminalset> first;
	std::map<std::string, std::vector<Terminal::t_terminalset>> first_per_rule;
	for(const NonTerminalPtr& nonterminal : all_nonterminals)
		calc_first(nonterminal, first, &first_per_rule);

	for(const auto& pair : first)
	{
		std::cout << pair.first << ": ";
		for(const auto& _first : pair.second)
			std::cout << _first->GetId() << ", ";
		std::cout << "\n";
	}
	std::cout << std::endl;


	std::cout << "FOLLOW sets:\n";
	std::map<std::string, Terminal::t_terminalset> follow;
	for(const NonTerminalPtr& nonterminal : all_nonterminals)
		calc_follow(all_nonterminals, start, nonterminal, first, follow);

	for(const auto& pair : follow)
	{
		std::cout << pair.first << ": ";
		for(const auto& _first : pair.second)
			std::cout << _first->GetId() << ", ";
		std::cout << "\n";
	}
	std::cout << std::endl;


	ElementPtr elem = std::make_shared<Element>(start, 0, 0, Terminal::t_terminalset{{g_end}, Terminal::terminals_compare});
	ClosurePtr coll = std::make_shared<Closure>();
	coll->AddElement(elem);
	//std::cout << "\n\n" << *coll << std::endl;

	Collection colls{coll};
	colls.DoTransitions();
	colls.WriteGraph("test1_lr", 0);
	colls.WriteGraph("test1_lr_full", 1);
	std::cout << "\n\nLR(1):\n" << colls << std::endl;

	Collection collsLALR = colls.ConvertToLALR();
	collsLALR.WriteGraph("test1_lalr", 0);
	collsLALR.WriteGraph("test1_lalr_full", 1);
	std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;

	Collection collsSLR = colls.ConvertToSLR(follow);
	collsSLR.WriteGraph("test1_slr", 0);
	collsSLR.WriteGraph("test1_slr_full", 1);
	std::cout << "\n\nSLR(1):\n" << collsSLR << std::endl;

	return 0;
}
