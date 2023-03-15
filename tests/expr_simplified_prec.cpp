/**
 * operator precedence and associativity test
 * (solve conflicts on table generation)
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 05-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "parsergen/lr1.h"
#include "codegen/lexer.h"
#include "codegen/parser.h"
#include "codegen/ast.h"
#include "codegen/ast_printer.h"

#include <iostream>
#include <sstream>
#include <iomanip>


enum : std::size_t
{
	START,
	EXPR,
};


#define USE_LOOKBACK_LOOKAHEAD_SOLUTIONS


int main()
{
	try
	{
		auto start = std::make_shared<NonTerminal>(START, "start");
		auto expr = std::make_shared<NonTerminal>(EXPR, "expr");

		auto plus = std::make_shared<Terminal>('+', "+");
		auto mult = std::make_shared<Terminal>('*', "*");
		auto bracket_open = std::make_shared<Terminal>('(', "(");
		auto bracket_close = std::make_shared<Terminal>(')', ")");
		auto sym = std::make_shared<Terminal>((std::size_t)Token::REAL, "symbol");


		std::size_t semanticindex = 0;

		// rule 0
		start->AddRule({ expr }, semanticindex++);
		// rule 1
		expr->AddRule({ expr, plus, expr }, semanticindex++);
		// rule 2
		expr->AddRule({ expr, mult, expr }, semanticindex++);
		// rule 3
		expr->AddRule({ sym }, semanticindex++);
		// rule 4
		expr->AddRule({ bracket_open, expr, bracket_close }, semanticindex++);

		std::vector<NonTerminalPtr> all_nonterminals{{ start, expr }};

		std::cout << "Productions:\n";
		for(NonTerminalPtr nonterm : all_nonterminals)
			nonterm->print(std::cout);
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
			calc_follow(all_nonterminals, start, nonterminal, first, follow);

		for(const auto& pair : follow)
		{
			std::cout << pair.first->GetStrId() << ": ";
			for(const auto& _first : pair.second)
				std::cout << _first->GetStrId() << ", ";
			std::cout << "\n";
		}
		std::cout << std::endl;


		ElementPtr elem = std::make_shared<Element>(start, 0, 0,
			Terminal::t_terminalset{{g_end}});
		ClosurePtr coll = std::make_shared<Closure>();
		coll->AddElement(elem);

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

		// solutions for conflicts
		std::vector<t_conflictsolution> conflicts{{
#ifdef USE_LOOKBACK_LOOKAHEAD_SOLUTIONS
			// left-associativity of operators
			std::make_tuple(mult, mult, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(plus, plus, ConflictSolution::FORCE_REDUCE),

			// operator precedence
			std::make_tuple(mult, plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(plus, mult, ConflictSolution::FORCE_SHIFT),
#else
			std::make_tuple(expr, plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(expr, mult, ConflictSolution::FORCE_SHIFT),
#endif
		}};

		auto parsetables = collsLALR.CreateParseTables(&conflicts);
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
			[&mapNonTermIdx, &expr, &plus](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], plus->GetId());
			},

			// rule 2
			[&mapNonTermIdx, &expr, &mult](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], mult->GetId());
			},

			// rule 3
			[&mapNonTermIdx, &expr](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 4
			[&mapNonTermIdx, &expr](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[1]);
			},
		}};


		Parser parser{parsetables, rules};

		{
			// test operator precedence
			std::string exprstr{"(2.*3. + (5.+4.) * (1.+2.)) * 5.+12."};
			std::istringstream istr{exprstr};
			auto tokens = get_all_tokens(istr, &mapTermIdx);

			auto ast = ASTBase::cst_to_ast(parser.Parse(tokens));
			std::cout << "AST for expression " << exprstr << ":\n";
			ASTPrinter printer{std::cout};
			ast->accept(&printer);
		}

		{
			// test associativity -> forced reduce -> left associative
			std::string exprstr{"1. + 2. + 3. + 4. + 5."};
			std::istringstream istr{exprstr};
			auto tokens = get_all_tokens(istr, &mapTermIdx);

			auto ast = ASTBase::cst_to_ast(parser.Parse(tokens));
			std::cout << "\nAST for expression " << exprstr << ":\n";
			ASTPrinter printer{std::cout};
			ast->accept(&printer);
		}

		{
			// test associativity -> forced shift -> right associative
			std::string exprstr{"1. * 2. * 3. * 4. * 5."};
			std::istringstream istr{exprstr};
			auto tokens = get_all_tokens(istr, &mapTermIdx);

			auto ast = ASTBase::cst_to_ast(parser.Parse(tokens));
			std::cout << "\nAST for expression " << exprstr << ":\n";
			ASTPrinter printer{std::cout};
			ast->accept(&printer);
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
