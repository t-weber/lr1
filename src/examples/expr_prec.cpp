/**
 * expression test with operator precedences/associativities
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 06-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "parsergen/lr1.h"
#include "codegen/lexer.h"
#include "codegen/ast.h"
#include "codegen/ast_printer.h"
#include "codegen/ast_asm.h"
#include "vm/vm.h"

#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>


#define USE_LALR          1
#define DEBUG_PARSERGEN   1
#define DEBUG_WRITEGRAPH  0
#define DEBUG_CODEGEN     1


enum : std::size_t
{
	START,
	EXPR,
};


static NonTerminalPtr start, expr;

static TerminalPtr op_plus, op_minus, op_mult, op_div, op_mod, op_pow;
static TerminalPtr bracket_open, bracket_close, comma;
static TerminalPtr sym, ident;


static void create_grammar()
{
	start = std::make_shared<NonTerminal>(START, "start");
	expr = std::make_shared<NonTerminal>(EXPR, "expr");

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

	// rule 0: start -> expr
	start->AddRule({ expr }, semanticindex++);
	// rule 1: expr -> expr + expr
	expr->AddRule({ expr, op_plus, expr }, semanticindex++);
	// rule 2: expr -> expr - expr
	expr->AddRule({ expr, op_minus, expr }, semanticindex++);
	// rule 3: expr -> expr * expr
	expr->AddRule({ expr, op_mult, expr }, semanticindex++);
	// rule 4: expr -> expr / expr
	expr->AddRule({ expr, op_div, expr }, semanticindex++);
	// rule 5: expr -> expr % expr
	expr->AddRule({ expr, op_mod, expr }, semanticindex++);
	// rule 6: expr -> expr ^ expr
	expr->AddRule({ expr, op_pow, expr }, semanticindex++);
	// rule 7: expr -> ( expr )
	expr->AddRule({ bracket_open, expr, bracket_close }, semanticindex++);
	// function calls
	// rule 8: expr -> ident()
	expr->AddRule({ ident, bracket_open, bracket_close }, semanticindex++);
	// rule 9: expr -> ident(expr)
	expr->AddRule({ ident, bracket_open, expr, bracket_close }, semanticindex++);
	// rule 10: expr -> ident(expr, expr)
	expr->AddRule({ ident, bracket_open, expr, comma, expr, bracket_close }, semanticindex++);
	// rule 11: expr -> symbol
	expr->AddRule({ sym }, semanticindex++);
	// rule 12: expr -> ident
	expr->AddRule({ ident }, semanticindex++);
	// rule 13, unary-: expr -> -expr
	expr->AddRule({ op_minus, expr }, semanticindex++);
	// rule 14, unary+: expr -> +expr
	expr->AddRule({ op_plus, expr }, semanticindex++);
}



#ifdef CREATE_PARSER

static void lr1_create_parser()
{
	try
	{
#if DEBUG_PARSERGEN != 0
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
#endif


		ElementPtr elem = std::make_shared<Element>(
			start, 0, 0, Terminal::t_terminalset{{g_end}});
		ClosurePtr closure = std::make_shared<Closure>();
		closure->SetThisPtr(closure);
		closure->AddElement(elem);
		//std::cout << "\n\n" << *closure << std::endl;


#if USE_LALR != 0
		/*Collection colls{ closure };
		colls.DoTransitions();
		Collection collsLALR = colls.ConvertToLALR();*/

		Collection collsLALR{ closure };
		collsLALR.DoTransitions(false);
#else
		Collection colls{ closure };
		colls.DoTransitions();
#endif

#if DEBUG_PARSERGEN != 0
#if USE_LALR != 0
		std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;
#else
		std::cout << "\n\nLR(1):\n" << colls << std::endl;
#endif
#endif

#if DEBUG_WRITEGRAPH != 0
#if USE_LALR != 0
		collsLALR.WriteGraph("expr_prec_lalr_full", 1);
#else
		colls.WriteGraph("expr_prec_full", 1);
#endif
#endif

		// solutions for conflicts
		std::vector<t_conflictsolution> conflicts{{
			// left-associativity of operators
			std::make_tuple(op_plus, op_plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_minus, op_minus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mult, op_mult, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_div, op_div, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mod, op_mod, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_plus, op_minus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_minus, op_plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mult, op_div, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mult, op_mod, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_div, op_mult, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_div, op_mod, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mod, op_mult, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mod, op_div, ConflictSolution::FORCE_REDUCE),

			// right-associativity of operators
			std::make_tuple(op_pow, op_pow, ConflictSolution::FORCE_SHIFT),

			// operator precedence
			std::make_tuple(op_mult, op_plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mult, op_minus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_div, op_plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_div, op_minus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mod, op_plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_mod, op_minus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_plus, op_mult, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_minus, op_mult, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_plus, op_div, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_minus, op_div, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_plus, op_mod, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_minus, op_mod, ConflictSolution::FORCE_SHIFT),

			std::make_tuple(op_pow, op_plus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_pow, op_minus, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_pow, op_mult, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_pow, op_div, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_pow, op_mod, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_plus, op_pow, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_minus, op_pow, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_mult, op_pow, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_div, op_pow, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_mod, op_pow, ConflictSolution::FORCE_SHIFT),
        }};

#if USE_LALR != 0
		auto parsetables = collsLALR.CreateParseTables(&conflicts);
		collsLALR.SaveParseTables(parsetables, "expr_prec.tab");
#else
		auto parsetables = colls.CreateParseTables(&conflicts);
		colls.SaveParseTables(parsetables, "expr_prec.tab");
#endif
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
	}
}

#endif



#ifdef RUN_PARSER

#if !__has_include("expr_prec.tab")

static void lr1_run_parser()
{
	std::cerr << "No parsing tables available, please run ./expr_prec_create first and rebuild."
		<< std::endl;
}

#else

#include "codegen/parser.h"
#include "expr_prec.tab"

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
			// rule 0: start -> expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = start->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 1: expr -> expr + expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_plus->GetId());
			},

			// rule 2: expr -> expr - expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_minus->GetId());
			},

			// rule 3: expr -> expr * expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_mult->GetId());
			},

			// rule 4: expr -> expr / expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_div->GetId());
			},

			// rule 5: expr -> expr % expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], op_mod->GetId());
			},

			// rule 6: expr -> expr ^ expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_pow->GetId());
			},

			// rule 7: expr -> ( expr )
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[1]);
			},

			// rule 8: expr -> ident()
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 9: expr -> ident(expr)
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 10: expr -> ident(expr, expr)
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 11: expr -> symbol
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 12: expr -> ident
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 13, unary-: expr -> -expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(id, tableidx, args[1], op_minus->GetId());
			},

			// rule 14, unary+: expr -> +expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(id, tableidx, args[1], op_plus->GetId());
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

#if DEBUG_CODEGEN != 0
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
#endif

			auto ast = ASTBase::cst_to_ast(parser.Parse(tokens));

#if DEBUG_CODEGEN != 0
			std::cout << "AST:\n";
			ASTPrinter printer{std::cout};
			ast->accept(&printer);
#endif

			std::unordered_map<std::size_t, std::tuple<std::string, OpCode>> ops
			{{
				std::make_pair('+', std::make_tuple("add", OpCode::ADD)),
				std::make_pair('-', std::make_tuple("sub", OpCode::SUB)),
				std::make_pair('*', std::make_tuple("mul", OpCode::MUL)),
				std::make_pair('/', std::make_tuple("div", OpCode::DIV)),
				std::make_pair('%', std::make_tuple("mod", OpCode::MOD)),
				std::make_pair('^', std::make_tuple("pow", OpCode::POW)),
			}};

#if DEBUG_CODEGEN != 0
			std::ostringstream ostrAsm;
			ASTAsm astasm{ostrAsm, &ops};
			ast->accept(&astasm);
#endif

			std::ostringstream ostrAsmBin(
				std::ios_base::out | std::ios_base::binary);
			ASTAsm astasmbin{ostrAsmBin, &ops};
			astasmbin.SetBinary(true);
			ast->accept(&astasmbin);

#if DEBUG_CODEGEN != 0
			std::cout << "Generated code ("
				<< ostrAsmBin.str().size() << " bytes):\n"
				<< ostrAsm.str();
#endif

			VM vm(1024);
			vm.SetMem(0, ostrAsmBin.str());
			vm.Run();
			std::cout << "Result: " << vm.Top<t_real>() << std::endl;
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
	std::ios_base::sync_with_stdio(false);

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
