/**
 * ll1 comparison
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
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
	POW_TERM,

	FACTOR,

	SYMBOL
};


int main()
{
	auto start = std::make_shared<NonTerminal>(START, "start");
	auto add_term = std::make_shared<NonTerminal>(ADD_TERM, "add_term");
	auto mul_term = std::make_shared<NonTerminal>(MUL_TERM, "mul_term");
	auto pow_term = std::make_shared<NonTerminal>(POW_TERM, "pow_term");
	auto factor = std::make_shared<NonTerminal>(FACTOR, "factor");

	auto plus = std::make_shared<Terminal>('+', "+");
	auto minus = std::make_shared<Terminal>('-', "-");
	auto mult = std::make_shared<Terminal>('*', "*");
	auto div = std::make_shared<Terminal>('/', "/");
	auto mod = std::make_shared<Terminal>('%', "%");
	auto pow = std::make_shared<Terminal>('^', "^");
	auto bracket_open = std::make_shared<Terminal>('(', "(");
	auto bracket_close = std::make_shared<Terminal>(')', ")");
	auto sym = std::make_shared<Terminal>(SYMBOL, "symbol");


	std::size_t semanticindex = 0;

	start->AddRule({ add_term }, semanticindex++);				// rule 0

	add_term->AddRule({ plus, mul_term }, semanticindex++);			// rule 1
	add_term->AddRule({ minus, mul_term }, semanticindex++);		// rule 2
	add_term->AddRule({ add_term, plus, mul_term }, semanticindex++);	// rule 3
	add_term->AddRule({ add_term, minus, mul_term }, semanticindex++);	// rule 4
	add_term->AddRule({ mul_term }, semanticindex++);			// rule 5

	mul_term->AddRule({ mul_term, mult, pow_term }, semanticindex++);	// rule 6
	mul_term->AddRule({ mul_term, div, pow_term }, semanticindex++);	// rule 7
	mul_term->AddRule({ mul_term, mod, pow_term }, semanticindex++);	// rule 8
	mul_term->AddRule({ pow_term }, semanticindex++);			// rule 9

	pow_term->AddRule({ pow_term, pow, factor }, semanticindex++);		// rule 10
	pow_term->AddRule({ factor }, semanticindex++);				// rule 11

	factor->AddRule({ bracket_open, add_term, bracket_close }, semanticindex++);	// rule 12
	factor->AddRule({ sym }, semanticindex++);				// rule 13


	LL1 ll1{{start, add_term, mul_term, pow_term, factor}};
	ll1.RemoveLeftRecursion(1000, "_rest", &semanticindex);
	ll1.CalcFirstFollow();
	ll1.CalcTable();

	std::cout << ll1 << std::endl;
	std::cout << "Parser pseudo-code:\n";
	ll1.PrintRecursiveDescentPseudocode(std::cout);
	std::cout << std::endl;

	return 0;
}
