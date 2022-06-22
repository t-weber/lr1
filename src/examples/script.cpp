/**
 * scripting test
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 08-jun-2022
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
#include <fstream>
#include <iomanip>
#include <cstdint>


#define USE_LALR          1
#define DEBUG_PARSERGEN   1
#define DEBUG_WRITEGRAPH  0
#define DEBUG_CODEGEN     1
#define RUN_VM            1


/**
 * non-terminals identifiers
 */
enum : std::size_t
{
	START,      // start
	STMTS,      // list of statements
	STMT,       // statement
	EXPR,       // expression
	EXPRS,      // list of expressions
	BOOL_EXPR,  // boolean expression
	IDENTS,     // list of identifiers
};


/**
 * non-terminals
 */
static NonTerminalPtr start,
	stmts, stmt,
	exprs, expr,
	bool_expr, idents;


/**
 * terminals
 */
static TerminalPtr op_assign, op_plus, op_minus,
	op_mult, op_div, op_mod, op_pow,
	op_and, op_or, op_not,
	op_equ, op_nequ, op_lt, op_gt, op_gequ, op_lequ;
static TerminalPtr bracket_open, bracket_close;
static TerminalPtr block_begin, block_end;
static TerminalPtr keyword_if, keyword_else, keyword_loop,
	keyword_func;
static TerminalPtr comma, stmt_end;
static TerminalPtr sym, ident;


static void create_grammar()
{
	// non-terminals
	start = std::make_shared<NonTerminal>(START, "start");
	stmts = std::make_shared<NonTerminal>(STMTS, "stmts");
	stmt = std::make_shared<NonTerminal>(STMT, "stmt");
	exprs = std::make_shared<NonTerminal>(EXPRS, "exprs");
	expr = std::make_shared<NonTerminal>(EXPR, "expr");
	bool_expr = std::make_shared<NonTerminal>(BOOL_EXPR, "bool_expr");
	idents = std::make_shared<NonTerminal>(IDENTS, "idents");

	// terminals
	op_assign = std::make_shared<Terminal>('=', "=");
	op_plus = std::make_shared<Terminal>('+', "+");
	op_minus = std::make_shared<Terminal>('-', "-");
	op_mult = std::make_shared<Terminal>('*', "*");
	op_div = std::make_shared<Terminal>('/', "/");
	op_mod = std::make_shared<Terminal>('%', "%");
	op_pow = std::make_shared<Terminal>('^', "^");

	op_equ = std::make_shared<Terminal>(static_cast<std::size_t>(Token::EQU), "==");
	op_nequ = std::make_shared<Terminal>(static_cast<std::size_t>(Token::NEQU), "!=");
	op_gequ = std::make_shared<Terminal>(static_cast<std::size_t>(Token::GEQU), ">=");
	op_lequ = std::make_shared<Terminal>(static_cast<std::size_t>(Token::LEQU), "<=");
	op_gt = std::make_shared<Terminal>('>', ">");
	op_lt = std::make_shared<Terminal>('<', "<");
	op_not = std::make_shared<Terminal>('!', "!");
	op_and = std::make_shared<Terminal>('&', "&");
	op_or = std::make_shared<Terminal>('|', "|");

	bracket_open = std::make_shared<Terminal>('(', "(");
	bracket_close = std::make_shared<Terminal>(')', ")");
	block_begin = std::make_shared<Terminal>('{', "{");
	block_end = std::make_shared<Terminal>('}', "}");

	comma = std::make_shared<Terminal>(',', ",");
	stmt_end = std::make_shared<Terminal>(';', ";");

	sym = std::make_shared<Terminal>(static_cast<std::size_t>(Token::REAL), "symbol");
	ident = std::make_shared<Terminal>(static_cast<std::size_t>(Token::IDENT), "ident");

	keyword_if = std::make_shared<Terminal>(static_cast<std::size_t>(Token::IF), "if");
	keyword_else = std::make_shared<Terminal>(static_cast<std::size_t>(Token::ELSE), "else");
	keyword_loop = std::make_shared<Terminal>(static_cast<std::size_t>(Token::LOOP), "loop");
	keyword_func = std::make_shared<Terminal>(static_cast<std::size_t>(Token::FUNC), "func");

	// rule number
	std::size_t semanticindex = 0;

	// rule 0: start -> stmts
	start->AddRule({ stmts }, semanticindex++);

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
	// rule 8: function call, expr -> ident( exprs )
	expr->AddRule({ ident, bracket_open, exprs, bracket_close }, semanticindex++);
	// rule 9: expr -> symbol
	expr->AddRule({ sym }, semanticindex++);
	// rule 10: expr -> ident
	expr->AddRule({ ident }, semanticindex++);
	// rule 11, unary-: expr -> -expr
	expr->AddRule({ op_minus, expr }, semanticindex++);
	// rule 12, unary+: expr -> +expr
	expr->AddRule({ op_plus, expr }, semanticindex++);
	// rule 13, assignment: expr -> ident = expr
	expr->AddRule({ ident, op_assign, expr }, semanticindex++);

	// rule 14: stmts -> stmt stmts
	stmts->AddRule({ stmt, stmts }, semanticindex++);
	// rule 15: stmts -> eps
	stmts->AddRule({ g_eps }, semanticindex++);

	// rule 16: stmt -> expr ;
	stmt->AddRule({ expr, stmt_end }, semanticindex++);

	// rule 17: stmt -> if(bool_expr) { stmts }
	stmt->AddRule({ keyword_if, bracket_open, bool_expr, bracket_close,
		block_begin, stmts, block_end }, semanticindex++);
	// rule 18: stmt -> if(bool_expr) { stmts } else { stmts }
	stmt->AddRule({ keyword_if, bracket_open, bool_expr, bracket_close,
		block_begin, stmts, block_end,
		keyword_else, block_begin, stmts, block_end}, semanticindex++);
	// rule 19: stmt -> loop(bool_expr) { stmts }
	stmt->AddRule({ keyword_loop, bracket_open, bool_expr, bracket_close,
		block_begin, stmts, block_end }, semanticindex++);
	// rule 20: stmt -> func name ( idents ) { stmts }
	stmt->AddRule({ keyword_func, ident, bracket_open, idents, bracket_close,
		block_begin, stmts, block_end }, semanticindex++);

	// rule 21: bool_expr -> bool_expr & bool_expr
	bool_expr->AddRule({ bool_expr, op_and, bool_expr }, semanticindex++);
	// rule 22: bool_expr -> bool_expr | bool_expr
	bool_expr->AddRule({ bool_expr, op_or, bool_expr }, semanticindex++);
	// rule 23: bool_expr -> !bool_expr
	bool_expr->AddRule({ op_not, bool_expr, }, semanticindex++);
	// rule 24: bool_expr -> ( bool_expr )
	bool_expr->AddRule({ bracket_open, bool_expr, bracket_close }, semanticindex++);
	// rule 25: bool_expr -> expr > expr
	bool_expr->AddRule({ expr, op_gt, expr }, semanticindex++);
	// rule 26: bool_expr -> expr < expr
	bool_expr->AddRule({ expr, op_lt, expr }, semanticindex++);
	// rule 27: bool_expr -> expr >= expr
	bool_expr->AddRule({ expr, op_gequ, expr }, semanticindex++);
	// rule 28: bool_expr -> expr <= expr
	bool_expr->AddRule({ expr, op_lequ, expr }, semanticindex++);
	// rule 29: bool_expr -> expr == expr
	bool_expr->AddRule({ expr, op_equ, expr }, semanticindex++);
	// rule 30: bool_expr -> expr != expr
	bool_expr->AddRule({ expr, op_nequ, expr }, semanticindex++);

	// rule 31: idents -> ident, idents
	idents->AddRule({ ident, comma, idents }, semanticindex++);
	// rule 32: idents -> ident
	idents->AddRule({ ident }, semanticindex++);
	// rule 33: idents -> eps
	idents->AddRule({ g_eps }, semanticindex++);

	// rule 34: exprs -> expr, exprs
	exprs->AddRule({ expr, comma, exprs }, semanticindex++);
	// rule 35: exprs -> expr
	exprs->AddRule({ expr }, semanticindex++);
	// rule 36: exprs -> eps
	exprs->AddRule({ g_eps }, semanticindex++);
}



#ifdef CREATE_PARSER

static void lr1_create_parser()
{
	try
	{
#if DEBUG_PARSERGEN != 0
		std::vector<NonTerminalPtr> all_nonterminals{{
			start, stmts, stmt, exprs, expr, bool_expr, idents }};

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
			start, 0, 0, Terminal::t_terminalset{{ g_end }});
		ClosurePtr closure = std::make_shared<Closure>();
		closure->AddElement(elem);

		auto progress = [](const std::string& msg, [[maybe_unused]] bool done)
		{
			std::cout << "\r" << msg << "                ";
			if(done)
				std::cout << "\n";
			std::cout.flush();
		};

#if USE_LALR != 0
		//Collection colls{ closure };
		//colls.SetProgressObserver(progress);
		//colls.DoTransitions();
		//Collection collsLALR = colls.ConvertToLALR();

		Collection collsLALR{ closure };
		collsLALR.SetProgressObserver(progress);
		collsLALR.DoTransitions(false);
#else
		Collection colls{ closure };
		colls.SetProgressObserver(progress);
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
		collsLALR.WriteGraph("script_lalr", 1);
#else
		colls.WriteGraph("script", 1);
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
			std::make_tuple(op_and, op_and, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_or, op_or, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_not, op_not, ConflictSolution::FORCE_REDUCE),

			// right-associativity of operators
			std::make_tuple(op_pow, op_pow, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_assign, ConflictSolution::FORCE_SHIFT),

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
			std::make_tuple(op_and, op_or, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_or, op_and, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_and, op_not, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_or, op_not, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_not, op_and, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_not, op_or, ConflictSolution::FORCE_SHIFT),

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

			std::make_tuple(op_assign, op_plus, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_minus, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_mult, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_div, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_mod, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_pow, ConflictSolution::FORCE_SHIFT),
        }};

#if USE_LALR != 0
		auto parsetables = collsLALR.CreateParseTables(&conflicts);
		collsLALR.SaveParseTables(parsetables, "script.tab");
#else
		auto parsetables = colls.CreateParseTables(&conflicts);
		colls.SaveParseTables(parsetables, "script.tab");
#endif
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
	}
}

#endif



#ifdef RUN_PARSER

#if !__has_include("script.tab")

static bool lr1_run_parser([[maybe_unused]] const char* script_file = nullptr)
{
	std::cerr << "No parsing tables available, please run ./script_create first and rebuild."
		<< std::endl;
	return false;
}

#else

#include "codegen/parser.h"
#include "script.tab"

static bool lr1_run_parser(const char* script_file = nullptr)
{
	try
	{
		// get created parsing tables
		auto parsetables = get_lr1_tables();

		const t_mapIdIdx& mapTermIdx = *std::get<3>(parsetables);
		const t_mapIdIdx& mapNonTermIdx = *std::get<4>(parsetables);


		// semantic rules for the grammar
		std::vector<t_semanticrule> rules{{
			// rule 0: start -> stmts
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
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_plus->GetId());
			},

			// rule 2: expr -> expr - expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_minus->GetId());
			},

			// rule 3: expr -> expr * expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_mult->GetId());
			},

			// rule 4: expr -> expr / expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_div->GetId());
			},

			// rule 5: expr -> expr % expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_mod->GetId());
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

			// rule 8: function call, expr -> ident ( exprs )
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				if(args[0]->GetType() != ASTType::TOKEN)
					throw std::runtime_error("Expected a function mame.");

				auto funcname = std::dynamic_pointer_cast<ASTToken<std::string>>(args[0]);
				const std::string& ident = funcname->GetLexerValue();

				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTFuncCall>(id, tableidx, ident, args[2]);
			},

			// rule 9: expr -> symbol
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 10: expr -> ident
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 11, unary-: expr -> -expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(
					id, tableidx, args[1], op_minus->GetId());
			},

			// rule 12, unary+: expr -> +expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(
					id, tableidx, args[1], op_plus->GetId());
			},

			// rule 13, assignment: expr -> ident = expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				if(args[0]->GetType() != ASTType::TOKEN)
					throw std::runtime_error(
						"Expected a symbol name on lhs of assignment.");

				auto symname = std::dynamic_pointer_cast<ASTToken<std::string>>(args[0]);
				symname->SetLValue(true);

				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[2], args[0], op_assign->GetId());
			},

			// rule 14: stmts -> stmt stmts
			[](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				auto stmts_lst = std::dynamic_pointer_cast<ASTList>(args[1]);
				stmts_lst->AddChild(args[0], true);
				return stmts_lst;
			},

			// rule 15, stmts -> eps
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmts->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTList>(id, tableidx);
			},

			// rule 16, stmt -> expr ;
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 17: stmt -> if(bool_expr) { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTCondition>(
					id, tableidx, args[2], args[5]);
			},

			// rule 18: stmt -> if(bool_expr) { stmts } else { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTCondition>(
					id, tableidx, args[2], args[5], args[9]);
			},

			// rule 19: stmt -> loop(bool_expr) { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTLoop>(
					id, tableidx, args[2], args[5]);
			},

			// rule 20: funcion, stmt -> func name ( idents ) { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				if(args[1]->GetType() != ASTType::TOKEN)
					throw std::runtime_error("Expected a function mame.");

				auto funcname = std::dynamic_pointer_cast<ASTToken<std::string>>(args[1]);
				const std::string& ident = funcname->GetLexerValue();

				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTFunc>(
					id, tableidx, ident, args[3], args[6]);
			},

			// rule 21: bool_expr -> bool_expr & bool_expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_and->GetId());
			},

			// rule 22: bool_expr -> bool_expr | bool_expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_or->GetId());
			},

			// rule 23: bool_expr -> !bool_expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(
					id, tableidx, args[1], op_not->GetId());
			},

			// rule 24: bool_expr -> ( bool_expr )
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[1]);
			},

			// rule 25: bool_expr -> expr > expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_gt->GetId());
			},

			// rule 26: bool_expr -> expr < expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_lt->GetId());
			},

			// rule 27: bool_expr -> expr >= expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_gequ->GetId());
			},

			// rule 28: bool_expr -> expr <= expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_lequ->GetId());
			},

			// rule 29: bool_expr -> expr == expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_equ->GetId());
			},

			// rule 30: bool_expr -> expr != expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_nequ->GetId());
			},

			// rule 31: idents -> ident, idents
			[](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				auto idents_lst = std::dynamic_pointer_cast<ASTList>(args[2]);
				idents_lst->AddChild(args[0], true);
				return idents_lst;
			},

			// rule 32: idents -> ident
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = idents->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				auto idents_lst = std::make_shared<ASTList>(id, tableidx);

				idents_lst->AddChild(args[0], true);
				return idents_lst;
			},

			// rule 33, idents -> eps
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = idents->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTList>(id, tableidx);
			},

			// rule 34: exprs -> expr, exprs
			[](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				auto exprs_lst = std::dynamic_pointer_cast<ASTList>(args[2]);
				exprs_lst->AddChild(args[0], false);
				return exprs_lst;
			},

			// rule 35: exprs -> expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = exprs->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				auto exprs_lst = std::make_shared<ASTList>(id, tableidx);

				exprs_lst->AddChild(args[0], false);
				return exprs_lst;
			},

			// rule 36, exprs -> eps
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = exprs->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTList>(id, tableidx);
			},
		}};


		Parser parser{parsetables, rules};

		bool loop_input = true;
		while(loop_input)
		{
			std::unique_ptr<std::istream> istr;

			if(script_file)
			{
				// read script from file
				istr = std::make_unique<std::ifstream>(script_file);
				loop_input = false;

				if(!*istr)
				{
					std::cerr << "Error: Cannot open file \""
						<< script_file << "\"." << std::endl;
					return false;
				}

				std::cout << "Running \"" << script_file << "\"." << std::endl;
			}
			else
			{
				// read statements from command line
				std::cout << "\nStatement: ";
				std::string script;
				std::getline(std::cin, script);
				istr = std::make_unique<std::istringstream>(script);
			}

			// tokenise script
			bool end_on_newline = (script_file == nullptr);
			auto tokens = get_all_tokens(*istr, &mapTermIdx, end_on_newline);

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

				std::make_pair('=', std::make_tuple("wrmem", OpCode::WRMEM)),

				std::make_pair(static_cast<std::size_t>(Token::EQU),
					std::make_tuple("equ", OpCode::EQU)),
				std::make_pair(static_cast<std::size_t>(Token::NEQU),
					std::make_tuple("nequ", OpCode::NEQU)),
				std::make_pair(static_cast<std::size_t>(Token::GEQU),
					std::make_tuple("gequ", OpCode::GEQU)),
				std::make_pair(static_cast<std::size_t>(Token::LEQU),
					std::make_tuple("lequ", OpCode::LEQU)),
				std::make_pair('>', std::make_tuple("gt", OpCode::GT)),
				std::make_pair('<', std::make_tuple("lt", OpCode::LT)),
				std::make_pair('&', std::make_tuple("and", OpCode::AND)),
				std::make_pair('|', std::make_tuple("or", OpCode::OR)),
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
			std::string strAsmBin = ostrAsmBin.str();

#if DEBUG_CODEGEN != 0
			std::cout << "Generated code ("
				<< strAsmBin.size() << " bytes):\n"
				<< ostrAsm.str();
#endif

			std::string binfile{"script.bin"};
			std::ofstream ofstrAsmBin(binfile, std::ios_base::binary);
			if(!ofstrAsmBin)
			{
				std::cerr << "Cannot open \""
					<< binfile << "\"." << std::endl;
				return false;
			}
			ofstrAsmBin.write(strAsmBin.data(), strAsmBin.size());
			if(ofstrAsmBin.fail())
			{
				std::cerr << "Cannot write \""
					<< binfile << "\"." << std::endl;
				return false;
			}

			std::cout << "Created compiled program \""
				<< binfile << "\"." << std::endl;

#if RUN_VM != 0
			VM vm(4096);
			vm.SetMem(0, strAsmBin);
			vm.Run();

			std::cout << "Result: ";
			std::visit([](auto&& val) -> void
			{
				std::cout << val;
			}, vm.TopData());
			std::cout << std::endl;
#endif
		}
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return false;
	}

	return true;
}

#endif
#endif



int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	std::ios_base::sync_with_stdio(false);

#ifdef CREATE_PARSER
	create_grammar();
	lr1_create_parser();
#endif

#ifdef RUN_PARSER
	const char* script_file = nullptr;
	if(argc >= 2)
			script_file = argv[1];

	create_grammar();
	lr1_run_parser(script_file);
#endif

	return 0;
}
