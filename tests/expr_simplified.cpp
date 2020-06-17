/**
 * test
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 * 	- expression grammar from: https://de.wikipedia.org/wiki/LL(k)-Grammatik#Beispiel
 */

#include "lr1.h"
#include "lexer.h"
#include "parser.h"

#include <iostream>
#include <sstream>
#include <iomanip>


enum : std::size_t
{
	START,
	ADD_TERM,
	MUL_TERM,
	FACTOR,
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
	auto sym = std::make_shared<Terminal>((std::size_t)Token::REAL, "symbol");


	std::size_t semanticindex = 0;

	// rule 0
	start->AddRule({ add_term }, semanticindex++);
	// rule 1
	add_term->AddRule({ add_term, plus, mul_term }, semanticindex++);
	// rule 2
	add_term->AddRule({ mul_term }, semanticindex++);
	// rule 3
	mul_term->AddRule({ mul_term, mult, factor }, semanticindex++);
	// rule 4
	mul_term->AddRule({ factor }, semanticindex++);
	// rule 5
	factor->AddRule({ bracket_open, add_term, bracket_close }, semanticindex++);
	// rule 6
	factor->AddRule({ sym }, semanticindex++);

	std::vector<NonTerminalPtr> all_nonterminals{{start, add_term, mul_term, factor}};

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
			std::cout << _first->GetStrId() << ", ";
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
			std::cout << _first->GetStrId() << ", ";
		std::cout << "\n";
	}
	std::cout << std::endl;


	ElementPtr elem = std::make_shared<Element>(start, 0, 0, Terminal::t_terminalset{{g_end}, Terminal::terminals_compare});
	ClosurePtr coll = std::make_shared<Closure>();
	coll->AddElement(elem);
	//std::cout << "\n\n" << *coll << std::endl;

	Collection colls{coll};
	colls.DoTransitions();
	colls.WriteGraph("expr_simplified_lr", 0);
	colls.WriteGraph("expr_simplified_full", 1);
	std::cout << "\n\nLR(1):\n" << colls << std::endl;

	Collection collsLALR = colls.ConvertToLALR();
	collsLALR.WriteGraph("expr_simplified_lalr", 0);
	collsLALR.WriteGraph("expr_simplified_lalr_full", 1);
	std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;

	Collection collsSLR = colls.ConvertToSLR(follow);
	collsSLR.WriteGraph("expr_simplified_slr", 0);
	collsSLR.WriteGraph("expr_simplified_slr_full", 1);
	std::cout << "\n\nSLR(1):\n" << collsSLR << std::endl;


	auto parsetables = collsLALR.CreateParseTables();
	const t_mapIdIdx& mapTermIdx = std::get<3>(parsetables);
	const t_mapIdIdx& mapNonTermIdx = std::get<4>(parsetables);


	// rules for simplified grammar
	std::vector<t_semanticrule> rules{{
		// rule 0
		[&mapNonTermIdx, &start](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
		{
			std::size_t id = start->GetId();
			std::size_t tableidx = mapNonTermIdx.find(id)->second;
			return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
		},

		// rule 1
		[&mapNonTermIdx, &add_term, &plus](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
		{
			std::size_t id = add_term->GetId();
			std::size_t tableidx = mapNonTermIdx.find(id)->second;
			return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], plus->GetId());
		},

		// rule 2
		[&mapNonTermIdx, &add_term](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
		{
			std::size_t id = add_term->GetId();
			std::size_t tableidx = mapNonTermIdx.find(id)->second;
			return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
		},

		// rule 3
		[&mapNonTermIdx, &mul_term, &mult](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
		{
			std::size_t id = mul_term->GetId();
			std::size_t tableidx = mapNonTermIdx.find(id)->second;
			return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], mult->GetId());
		},

		// rule 4
		[&mapNonTermIdx, &mul_term](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
		{
			std::size_t id = mul_term->GetId();
			std::size_t tableidx = mapNonTermIdx.find(id)->second;
			return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
		},

		// rule 5
		[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
		{
			std::size_t id = factor->GetId();
			std::size_t tableidx = mapNonTermIdx.find(id)->second;
			return std::make_shared<ASTDelegate>(id, tableidx, args[1]);
		},

		// rule 6
		[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
		{
			std::size_t id = factor->GetId();
			std::size_t tableidx = mapNonTermIdx.find(id)->second;
			return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
		},
	}};


	Parser parser{parsetables, rules};

	std::string exprstr{"(2*3 + (5+4) * (1+2)) * 5+12"};
	std::istringstream istr{exprstr};
	auto tokens = get_all_tokens(istr, &mapTermIdx);

	auto ast = cst_to_ast(parser.Parse(tokens));
	std::cout << "AST for expression " << exprstr << ":\n";
	ast->print(std::cout);

	return 0;
}
