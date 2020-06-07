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
	constexpr int example = 0;
	constexpr bool bSimplifiedGrammar = 0;

	if constexpr(example == 0)
	{
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

		start->AddRule({ add_term });

		add_term->AddRule({ add_term, plus, mul_term });
		if(!bSimplifiedGrammar)
			add_term->AddRule({ add_term, minus, mul_term });
		add_term->AddRule({ mul_term });

		if(bSimplifiedGrammar)
		{
			mul_term->AddRule({ mul_term, mult, factor });
		}
		else
		{
			mul_term->AddRule({ mul_term, mult, pow_term });
			mul_term->AddRule({ mul_term, div, pow_term });
			mul_term->AddRule({ mul_term, mod, pow_term });
		}

		if(bSimplifiedGrammar)
			mul_term->AddRule({ factor });
		else
			mul_term->AddRule({ pow_term });

		pow_term->AddRule({ pow_term, pow, factor });
		pow_term->AddRule({ factor });

		factor->AddRule({ bracket_open, add_term, bracket_close });
		if(!bSimplifiedGrammar)
		{
			factor->AddRule({ ident, bracket_open, bracket_close });			// function call
			factor->AddRule({ ident, bracket_open, add_term, bracket_close });			// function call
			factor->AddRule({ ident, bracket_open, add_term, comma, add_term, bracket_close });	// function call
		}
		factor->AddRule({ sym });


		std::map<std::string, std::set<SymbolPtr>> first;
		std::map<std::string, std::vector<std::set<SymbolPtr>>> first_per_rule;
		calc_first(start, first, &first_per_rule);

		for(const auto& pair : first)
		{
			std::cout << pair.first << ": ";
			for(const auto& _first : pair.second)
				std::cout << _first->GetId() << ", ";
			std::cout << "\n";
		}


		// test
		//Element elem{factor, 0, 0, {g_end}};
		//std::cout << elem << std::endl;
	}

	else if constexpr(example == 1)
	{
		auto start = std::make_shared<NonTerminal>("start");

		auto A = std::make_shared<NonTerminal>("A");
		auto B = std::make_shared<NonTerminal>("B");

		auto a = std::make_shared<Terminal>("a");
		auto b = std::make_shared<Terminal>("b");

		start->AddRule({ A });

		A->AddRule({ a, b, B });
		A->AddRule({ b, b, B });
		B->AddRule({ b, B });

	}

	return 0;
}
