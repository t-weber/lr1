/**
 * ll1 comparison
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 * 	- expression grammar from: https://de.wikipedia.org/wiki/LL(k)-Grammatik#Beispiel
 */

#include "ll1.h"


enum : std::size_t
{
	START,
	ADD_TERM,
	MUL_TERM,
	FACTOR,

	SYMBOL
};


int main()
{
	auto start = std::make_shared<NonTerminal>(START, "start");
	auto add_term = std::make_shared<NonTerminal>(ADD_TERM, "add_term");
	auto mul_term = std::make_shared<NonTerminal>(MUL_TERM, "mul_term");
	auto factor = std::make_shared<NonTerminal>(FACTOR, "factor");

	auto plus = std::make_shared<Terminal>('+', "+");
	auto mult = std::make_shared<Terminal>('*', "*");
	auto bracket_open = std::make_shared<Terminal>('(', "(");
	auto bracket_close = std::make_shared<Terminal>(')', ")");
	auto sym = std::make_shared<Terminal>(SYMBOL, "symbol");


	std::size_t semanticindex = 0;

	start->AddRule({ add_term }, semanticindex++);	// rule 0
	add_term->AddRule({ add_term, plus, mul_term }, semanticindex++);	// rule 1
	add_term->AddRule({ mul_term }, semanticindex++);	// rule 2

	mul_term->AddRule({ mul_term, mult, factor }, semanticindex++);	// rule 3
	mul_term->AddRule({ factor }, semanticindex++);	// rule 4

	factor->AddRule({ bracket_open, add_term, bracket_close }, semanticindex++);	// rule 5
	factor->AddRule({ sym }, semanticindex++);	// rule 6


	LL1 ll1{{start, add_term, mul_term, factor}};
	ll1.RemoveLeftRecursion(1000, "_prime", &semanticindex);
	ll1.CalcFirstFollow();
	ll1.CalcTable();

	std::cout << ll1 << std::endl;
	std::cout << "Parser pseudo-code:\n";
	ll1.PrintRecursiveDescentPseudocode(std::cout);
	std::cout << std::endl;

	return 0;
}
