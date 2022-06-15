/**
 * expression test
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 * 	- expression grammar from: https://de.wikipedia.org/wiki/LL(k)-Grammatik#Beispiel
 */

#include "parsergen/lr1.h"
#include "codegen/lexer.h"
#include "codegen/ast_printer.h"

#include <iostream>
#include <sstream>
#include <iomanip>


enum : std::size_t
{
	START,
	ADD_TERM,
	MUL_TERM,
	POW_TERM,
	FACTOR
};


static NonTerminalPtr start, add_term, mul_term, pow_term, factor;

static TerminalPtr op_plus, op_minus, op_mult, op_div, op_mod, op_pow;
static TerminalPtr bracket_open, bracket_close, comma;
static TerminalPtr sym, ident;


static void create_grammar()
{
	start = std::make_shared<NonTerminal>(START, "start");
	add_term = std::make_shared<NonTerminal>(ADD_TERM, "add_term");
	mul_term = std::make_shared<NonTerminal>(MUL_TERM, "mul_term");
	pow_term = std::make_shared<NonTerminal>(POW_TERM, "pow_term");
	factor = std::make_shared<NonTerminal>(FACTOR, "factor");

	op_plus = std::make_shared<Terminal>('+', "+");
	op_minus = std::make_shared<Terminal>('-', "-");
	op_mult = std::make_shared<Terminal>('*', "*");
	op_div = std::make_shared<Terminal>('/', "/");
	op_mod = std::make_shared<Terminal>('%', "%");
	op_pow = std::make_shared<Terminal>('^', "^");
	bracket_open = std::make_shared<Terminal>('(', "(");
	bracket_close = std::make_shared<Terminal>(')', ")");
	comma = std::make_shared<Terminal>(',', ",");
	sym = std::make_shared<Terminal>((std::size_t)Token::REAL, "symbol");
	ident = std::make_shared<Terminal>((std::size_t)Token::IDENT, "ident");


	std::size_t semanticindex = 0;

	// rule 0
	start->AddRule({ add_term }, semanticindex++);
	// rule 1
	add_term->AddRule({ add_term, op_plus, mul_term }, semanticindex++);
	// rule 2
	add_term->AddRule({ add_term, op_minus, mul_term }, semanticindex++);
	// rule 3
	add_term->AddRule({ mul_term }, semanticindex++);
	// rule 4
	mul_term->AddRule({ mul_term, op_mult, pow_term }, semanticindex++);
	// rule 5
	mul_term->AddRule({ mul_term, op_div, pow_term }, semanticindex++);
	// rule 6
	mul_term->AddRule({ mul_term, op_mod, pow_term }, semanticindex++);
	// rule 7
	mul_term->AddRule({ pow_term }, semanticindex++);
	// rule 8
	pow_term->AddRule({ pow_term, op_pow, factor }, semanticindex++);
	// rule 9
	pow_term->AddRule({ factor }, semanticindex++);
	// rule 10
	factor->AddRule({ bracket_open, add_term, bracket_close }, semanticindex++);
	// function calls
	// rule 11
	factor->AddRule({ ident, bracket_open, bracket_close }, semanticindex++);
	// rule 12
	factor->AddRule({ ident, bracket_open, add_term, bracket_close }, semanticindex++);
	// rule 13
	factor->AddRule({ ident, bracket_open, add_term, comma, add_term, bracket_close }, semanticindex++);
	// rule 14
	factor->AddRule({ sym }, semanticindex++);
	// rule 15
	factor->AddRule({ ident }, semanticindex++);
}



#ifdef CREATE_PARSER

static void lr1_create_parser()
{
	try
	{
		std::vector<NonTerminalPtr> all_nonterminals{{
			start, add_term, mul_term, pow_term, factor}};

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


		ElementPtr elem = std::make_shared<Element>(start, 0, 0, Terminal::t_terminalset{{g_end}});
		ClosurePtr coll = std::make_shared<Closure>();
		coll->SetThisPtr(coll);
		coll->AddElement(elem);
		//std::cout << "\n\n" << *coll << std::endl;


		Collection colls{coll};
		colls.DoTransitions();
		std::cout << "\n\nLR(1):\n" << colls << std::endl;

		Collection collsLALR = colls.ConvertToLALR();
		collsLALR.WriteGraph("expr_lalr_full", 1);
		std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;


		auto parsetables = collsLALR.CreateParseTables();
		collsLALR.SaveParseTables(parsetables, "expr.tab");
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
	}
}

#endif



#ifdef RUN_PARSER

#if !__has_include("expr.tab")

static void lr1_run_parser()
{
	std::cerr << "No parsing tables available, please run ./expr_create first and rebuild."
		<< std::endl;
}

#else

#include "codegen/parser.h"
#include "expr.tab"

static void lr1_run_parser()
{
	try
	{
		// get created parsing tables
		auto parsetables = get_lr1_tables();

		const t_mapIdIdx& mapTermIdx = *std::get<3>(parsetables);
		const t_mapIdIdx& mapNonTermIdx = *std::get<4>(parsetables);


		// rules for simplified grammar
		std::vector<t_semanticrule> rules{{
			// rule 0
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = start->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 1
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = add_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_plus->GetId());
			},

			// rule 2
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = add_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_minus->GetId());
			},

			// rule 3
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = add_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 4
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_mult->GetId());
			},

			// rule 5
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_div->GetId());
			},

			// rule 6
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_mod->GetId());
			},

			// rule 7
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 8
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = pow_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_pow->GetId());
			},

			// rule 9
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = pow_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 10
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = factor->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[1]);
			},

			// rule 11
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 12
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 13
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 14
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = factor->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 15
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = factor->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},
		}};


		Parser parser{parsetables, rules};

		while(1)
		{
			std::string exprstr;
			std::cout << "\nExpression: ";
			std::getline(std::cin, exprstr);
			std::istringstream istr{exprstr};
			auto tokens = get_all_tokens(istr, &mapTermIdx);
			std::cout << "Tokens: ";
			for(const t_toknode& tok : tokens)
			{
				std::size_t tokid = tok->GetId();
				if(tokid == (std::size_t)Token::END)
					std::cout << "END";
				else
					std::cout << tokid;
				std::cout << " ";
			}
			std::cout << "\n";

			auto ast = ASTBase::cst_to_ast(parser.Parse(tokens));
			std::cout << "AST:\n";
			ASTPrinter printer{std::cout};
			ast->accept(&printer);
		}
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
	}
}

#endif
#endif



int main()
{
#ifdef CREATE_PARSER
	create_grammar();
	lr1_create_parser();
#endif

#ifdef RUN_PARSER
	create_grammar();
	lr1_run_parser();
#endif

	return 0;
}
