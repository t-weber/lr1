/**
 * scripting test
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 08-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "parsergen/lr1.h"
#include "parsergen/parsergen.h"
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
#include <chrono>
#include <cstdint>

#if __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#elif __has_include(<boost/filesystem.hpp>)
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#else
	#error No filesystem support found.
#endif

using t_clock = std::chrono::steady_clock;
using t_timepoint = std::chrono::time_point<t_clock>;
using t_duration = std::chrono::duration<t_real>;


#define USE_LALR          1
#define DEBUG_PARSERGEN   1
#define DEBUG_WRITEGRAPH  0
#define DEBUG_CODEGEN     1
#define WRITE_RECASC      0
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
	op_mult, op_div, op_mod, op_pow;
static TerminalPtr op_and, op_or, op_not,
	op_equ, op_nequ, op_lt, op_gt, op_gequ, op_lequ;
static TerminalPtr op_shift_left, op_shift_right;
static TerminalPtr op_binand, op_binor, op_binxor, op_binnot;
static TerminalPtr bracket_open, bracket_close;
static TerminalPtr block_begin, block_end;
static TerminalPtr keyword_if, keyword_else, keyword_loop,
	keyword_break, keyword_continue;
static TerminalPtr keyword_func, keyword_extern, keyword_return;
static TerminalPtr comma, stmt_end;
static TerminalPtr sym_real, sym_int, sym_str, ident;


static void create_symbols()
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
	op_and = std::make_shared<Terminal>(static_cast<std::size_t>(Token::AND), "&&");
	op_or = std::make_shared<Terminal>(static_cast<std::size_t>(Token::OR), "||");
	op_gt = std::make_shared<Terminal>('>', ">");
	op_lt = std::make_shared<Terminal>('<', "<");
	op_not = std::make_shared<Terminal>('!', "!");
	op_binand = std::make_shared<Terminal>('&', "&");
	op_binor = std::make_shared<Terminal>('|', "|");
	op_binnot = std::make_shared<Terminal>('~', "~");
	op_binxor = std::make_shared<Terminal>(static_cast<std::size_t>(Token::BIN_XOR), "xor");

	op_shift_left = std::make_shared<Terminal>(static_cast<std::size_t>(Token::SHIFT_LEFT), "<<");
	op_shift_right = std::make_shared<Terminal>(static_cast<std::size_t>(Token::SHIFT_RIGHT), ">>");

	bracket_open = std::make_shared<Terminal>('(', "(");
	bracket_close = std::make_shared<Terminal>(')', ")");
	block_begin = std::make_shared<Terminal>('{', "{");
	block_end = std::make_shared<Terminal>('}', "}");

	comma = std::make_shared<Terminal>(',', ",");
	stmt_end = std::make_shared<Terminal>(';', ";");

	sym_real = std::make_shared<Terminal>(static_cast<std::size_t>(Token::REAL), "real");
	sym_int = std::make_shared<Terminal>(static_cast<std::size_t>(Token::INT), "integer");
	sym_str = std::make_shared<Terminal>(static_cast<std::size_t>(Token::STR), "string");
	ident = std::make_shared<Terminal>(static_cast<std::size_t>(Token::IDENT), "ident");

	keyword_if = std::make_shared<Terminal>(static_cast<std::size_t>(Token::IF), "if");
	keyword_else = std::make_shared<Terminal>(static_cast<std::size_t>(Token::ELSE), "else");
	keyword_loop = std::make_shared<Terminal>(static_cast<std::size_t>(Token::LOOP), "loop");
	keyword_func = std::make_shared<Terminal>(static_cast<std::size_t>(Token::FUNC), "func");
	keyword_extern = std::make_shared<Terminal>(static_cast<std::size_t>(Token::EXTERN), "extern");
	keyword_return = std::make_shared<Terminal>(static_cast<std::size_t>(Token::RETURN), "return");
	keyword_continue = std::make_shared<Terminal>(static_cast<std::size_t>(Token::CONTINUE), "continue");
	keyword_break = std::make_shared<Terminal>(static_cast<std::size_t>(Token::BREAK), "break");
}



#ifdef CREATE_PARSER

static void create_grammar()
{
	// operator precedences and associativities
	// see: https://en.cppreference.com/w/c/language/operator_precedence
	op_assign->SetPrecedence(10);
	op_assign->SetAssociativity('r');

	op_or->SetPrecedence(20);
	op_and->SetPrecedence(21);
	op_or->SetAssociativity('l');
	op_and->SetAssociativity('l');

	op_binor->SetPrecedence(30);
	op_binxor->SetPrecedence(31);
	op_binand->SetPrecedence(32);
	op_binor->SetAssociativity('l');
	op_binxor->SetAssociativity('l');
	op_binand->SetAssociativity('l');

	op_equ->SetPrecedence(40);
	op_nequ->SetPrecedence(40);
	op_equ->SetAssociativity('l');
	op_nequ->SetAssociativity('l');

	op_lt->SetPrecedence(50);
	op_gt->SetPrecedence(50);
	op_gequ->SetPrecedence(50);
	op_lequ->SetPrecedence(50);
	op_lt->SetAssociativity('l');
	op_gt->SetAssociativity('l');
	op_gequ->SetAssociativity('l');
	op_lequ->SetAssociativity('l');

	op_shift_left->SetPrecedence(60);
	op_shift_right->SetPrecedence(60);
	op_shift_left->SetAssociativity('l');
	op_shift_right->SetAssociativity('l');

	op_plus->SetPrecedence(70);
	op_minus->SetPrecedence(70);
	op_plus->SetAssociativity('l');
	op_minus->SetAssociativity('l');

	op_mult->SetPrecedence(80);
	op_div->SetPrecedence(80);
	op_mod->SetPrecedence(80);
	op_mult->SetAssociativity('l');
	op_div->SetAssociativity('l');
	op_mod->SetAssociativity('l');

	op_not->SetPrecedence(90);
	op_not->SetAssociativity('l');

	op_binnot->SetPrecedence(100);
	op_binnot->SetAssociativity('l');

	op_pow->SetPrecedence(110);
	op_pow->SetAssociativity('r');


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

	// rule 9: expr -> real symbol
	expr->AddRule({ sym_real }, semanticindex++);
	// rule 10: expr -> int symbol
	expr->AddRule({ sym_int }, semanticindex++);
	// rule 11: expr -> string symbol
	expr->AddRule({ sym_str }, semanticindex++);
	// rule 12: expr -> ident
	expr->AddRule({ ident }, semanticindex++);

	// rule 13, unary-: expr -> -expr
	expr->AddRule({ op_minus, expr }, semanticindex++);
	// rule 14, unary+: expr -> +expr
	expr->AddRule({ op_plus, expr }, semanticindex++);
	// rule 15, assignment: expr -> ident = expr
	expr->AddRule({ ident, op_assign, expr }, semanticindex++);

	// rule 16: stmts -> stmt stmts
	stmts->AddRule({ stmt, stmts }, semanticindex++);
	// rule 17: stmts -> eps
	stmts->AddRule({ g_eps }, semanticindex++);

	// rule 18: stmt -> expr ;
	stmt->AddRule({ expr, stmt_end }, semanticindex++);

	// rule 19: stmt -> if(bool_expr) { stmts }
	stmt->AddRule({ keyword_if, bracket_open, bool_expr, bracket_close,
		block_begin, stmts, block_end }, semanticindex++);
	// rule 20: stmt -> if(bool_expr) { stmts } else { stmts }
	stmt->AddRule({ keyword_if, bracket_open, bool_expr, bracket_close,
		block_begin, stmts, block_end,
		keyword_else, block_begin, stmts, block_end}, semanticindex++);

	// rule 21: stmt -> loop(bool_expr) { stmts }
	stmt->AddRule({ keyword_loop, bracket_open, bool_expr, bracket_close,
		block_begin, stmts, block_end }, semanticindex++);
	// rule 22: stmt -> func name ( idents ) { stmts }
	stmt->AddRule({ keyword_func, ident, bracket_open, idents, bracket_close,
		block_begin, stmts, block_end }, semanticindex++);
	// rule 23: stmt -> extern func idents ;
	stmt->AddRule({ keyword_extern, keyword_func, idents, stmt_end }, semanticindex++);
	// rule 24: stmt -> break ;
	stmt->AddRule({ keyword_break, stmt_end }, semanticindex++);
	// rule 25: stmt -> break symbol ;
	stmt->AddRule({ keyword_break, sym_int, stmt_end }, semanticindex++);
	// rule 26: stmt -> continue ;
	stmt->AddRule({ keyword_continue, stmt_end }, semanticindex++);
	// rule 27: stmt -> continue symbol ;
	stmt->AddRule({ keyword_continue, sym_int, stmt_end }, semanticindex++);
	// rule 28: stmt -> return ;
	stmt->AddRule({ keyword_return, stmt_end }, semanticindex++);
	// rule 29: stmt -> return expr ;
	stmt->AddRule({ keyword_return, expr, stmt_end }, semanticindex++);

	// rule 30: bool_expr -> bool_expr and bool_expr
	bool_expr->AddRule({ bool_expr, op_and, bool_expr }, semanticindex++);
	// rule 31: bool_expr -> bool_expr or bool_expr
	bool_expr->AddRule({ bool_expr, op_or, bool_expr }, semanticindex++);
	// rule 32: bool_expr -> !bool_expr
	bool_expr->AddRule({ op_not, bool_expr, }, semanticindex++);
	// rule 33: bool_expr -> ( bool_expr )
	bool_expr->AddRule({ bracket_open, bool_expr, bracket_close }, semanticindex++);
	// rule 34: bool_expr -> expr > expr
	bool_expr->AddRule({ expr, op_gt, expr }, semanticindex++);
	// rule 35: bool_expr -> expr < expr
	bool_expr->AddRule({ expr, op_lt, expr }, semanticindex++);
	// rule 36: bool_expr -> expr >= expr
	bool_expr->AddRule({ expr, op_gequ, expr }, semanticindex++);
	// rule 37: bool_expr -> expr <= expr
	bool_expr->AddRule({ expr, op_lequ, expr }, semanticindex++);
	// rule 38: bool_expr -> expr == expr
	bool_expr->AddRule({ expr, op_equ, expr }, semanticindex++);
	// rule 39: bool_expr -> expr != expr
	bool_expr->AddRule({ expr, op_nequ, expr }, semanticindex++);

	// rule 40: idents -> ident, idents
	idents->AddRule({ ident, comma, idents }, semanticindex++);
	// rule 41: idents -> ident
	idents->AddRule({ ident }, semanticindex++);
	// rule 42: idents -> eps
	idents->AddRule({ g_eps }, semanticindex++);

	// rule 43: exprs -> expr, exprs
	exprs->AddRule({ expr, comma, exprs }, semanticindex++);
	// rule 44: exprs -> expr
	exprs->AddRule({ expr }, semanticindex++);
	// rule 45: exprs -> eps
	exprs->AddRule({ g_eps }, semanticindex++);

	// rule 46, binary not: expr -> ~expr
	expr->AddRule({ op_binnot, expr }, semanticindex++);
	// rule 47: expr -> expr bin_and expr
	expr->AddRule({ expr, op_binand, expr }, semanticindex++);
	// rule 48: expr -> expr bin_or expr
	expr->AddRule({ expr, op_binor, expr }, semanticindex++);
	// rule 49: expr -> expr bin_xor expr
	expr->AddRule({ expr, op_binxor, expr }, semanticindex++);
	// rule 50: expr -> expr << expr
	expr->AddRule({ expr, op_shift_left, expr }, semanticindex++);
	// rule 51: expr -> expr >> expr
	expr->AddRule({ expr, op_shift_right, expr }, semanticindex++);
}


static bool lr1_create_parser()
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

		// explicit solutions for conflicts
		/*std::vector<t_conflictsolution> conflicts{{
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
			std::make_tuple(op_binand, op_binand, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_binor, op_binor, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_binxor, op_binxor, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_binnot, op_binnot, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_shift_left, op_shift_left, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_shift_right, op_shift_right, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_shift_left, op_shift_right, ConflictSolution::FORCE_REDUCE),
			std::make_tuple(op_shift_right, op_shift_left, ConflictSolution::FORCE_REDUCE),

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
			std::make_tuple(op_assign, op_binand, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_binor, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_binxor, ConflictSolution::FORCE_SHIFT),
			std::make_tuple(op_assign, op_binnot, ConflictSolution::FORCE_SHIFT),
		}};*/

#if USE_LALR != 0
		auto parsetables = collsLALR.CreateParseTables(/*&conflicts*/);
		collsLALR.SaveParseTables(parsetables, "script.tab");
#else
		auto parsetables = colls.CreateParseTables(/*&conflicts*/);
		colls.SaveParseTables(parsetables, "script.tab");
#endif

#if WRITE_RECASC != 0
		ParserGen parsergen{parsetables};
		parsergen.CreateParser("script_parser.cpp");
#endif
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return false;
	}

	return true;
}

#endif  // CREATE_PARSER



#ifdef RUN_PARSER

#if !__has_include("script.tab")

static std::tuple<bool, std::string>
lr1_run_parser([[maybe_unused]] const char* script_file = nullptr)
{
	std::cerr << "No parsing tables available, please\n"
		"\t- run \"./script_create\" first,\n"
		"\t- \"touch " __FILE__ "\", and\n"
		"\t- rebuild using \"make\"."
		<< std::endl;

	return std::make_tuple(false, "");
}

#else

#include "codegen/parser.h"
#include "script.tab"

static std::tuple<bool, std::string>
lr1_run_parser(const char* script_file = nullptr)
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
					throw std::runtime_error("Expected a function name.");

				auto funcname = std::dynamic_pointer_cast<ASTToken<std::string>>(args[0]);
				funcname->SetIdent(true);
				const std::string& ident = funcname->GetLexerValue();

				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;

				auto funccall = std::make_shared<ASTFuncCall>(id, tableidx, ident, args[2]);
				funccall->SetLineRange(funcname->GetLineRange());
				return funccall;
			},

			// rule 9: expr -> real symbol
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				args[0]->SetDataType(VMType::REAL);
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 10: expr -> int symbol
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				args[0]->SetDataType(VMType::INT);
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 11: expr -> string symbol
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				args[0]->SetDataType(VMType::STR);
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 12: expr -> ident
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;

				auto ident = std::dynamic_pointer_cast<ASTToken<std::string>>(args[0]);
				ident->SetIdent(true);

				return std::make_shared<ASTDelegate>(id, tableidx, ident);
			},

			// rule 13, unary-: expr -> -expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(
					id, tableidx, args[1], op_minus->GetId());
			},

			// rule 14, unary+: expr -> +expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(
					id, tableidx, args[1], op_plus->GetId());
			},

			// rule 15, assignment: expr -> ident = expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				if(args[0]->GetType() != ASTType::TOKEN)
					throw std::runtime_error(
						"Expected a symbol name on lhs of assignment.");

				auto symname = std::dynamic_pointer_cast<ASTToken<std::string>>(args[0]);
				symname->SetIdent(true);
				symname->SetLValue(true);
				symname->SetDataType(args[2]->GetDataType());

				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[2], symname, op_assign->GetId());
			},

			// rule 16: stmts -> stmt stmts
			[](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				auto stmts_lst = std::dynamic_pointer_cast<ASTList>(args[1]);
				stmts_lst->AddChild(args[0], true);
				return stmts_lst;
			},

			// rule 17, stmts -> eps
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmts->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTList>(id, tableidx);
			},

			// rule 18, stmt -> expr ;
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 19: stmt -> if(bool_expr) { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTCondition>(
					id, tableidx, args[2], args[5]);
			},

			// rule 20: stmt -> if(bool_expr) { stmts } else { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTCondition>(
					id, tableidx, args[2], args[5], args[9]);
			},

			// rule 21: stmt -> loop(bool_expr) { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTLoop>(
					id, tableidx, args[2], args[5]);
			},

			// rule 22: funcion, stmt -> func name ( idents ) { stmts }
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				if(args[1]->GetType() != ASTType::TOKEN)
					throw std::runtime_error("Expected a function name.");

				auto funcname = std::dynamic_pointer_cast<ASTToken<std::string>>(args[1]);
				funcname->SetIdent(true);
				const std::string& ident = funcname->GetLexerValue();

				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;

				auto func = std::make_shared<ASTFunc>(id, tableidx, ident, args[3], args[6]);
				func->SetLineRange(funcname->GetLineRange());

				return func;
			},

			// rule 23: stmt -> extern func idents ;
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDeclare>(id, tableidx, true, true, args[2]);
			},

			// rule 24: stmt -> break ;
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;

				auto jump = std::make_shared<ASTJump>(
					id, tableidx, ASTJump::JumpType::BREAK);
				jump->SetLineRange(args[0]->GetLineRange());
				return jump;
			},

			// rule 25: stmt -> break symbol ;
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;

				return std::make_shared<ASTJump>(
					id, tableidx, ASTJump::JumpType::BREAK, args[1]);
			},

			// rule 26: stmt -> continue ;
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;

				auto jump = std::make_shared<ASTJump>(
					id, tableidx, ASTJump::JumpType::CONTINUE);
				jump->SetLineRange(args[0]->GetLineRange());
				return jump;
			},

			// rule 27: stmt -> continue symbol ;
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTJump>(
					id, tableidx, ASTJump::JumpType::CONTINUE, args[1]);
			},

			// rule 28: stmt -> return ;
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;

				auto jump = std::make_shared<ASTJump>(
					id, tableidx, ASTJump::JumpType::RETURN);
				jump->SetLineRange(args[0]->GetLineRange());
				return jump;
			},

			// rule 29: stmt -> return expr ;
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = stmt->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTJump>(
					id, tableidx, ASTJump::JumpType::RETURN, args[1]);
			},

			// rule 30: bool_expr -> bool_expr and bool_expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_and->GetId());
			},

			// rule 31: bool_expr -> bool_expr or bool_expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_or->GetId());
			},

			// rule 32: bool_expr -> !bool_expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(
					id, tableidx, args[1], op_not->GetId());
			},

			// rule 33: bool_expr -> ( bool_expr )
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[1]);
			},

			// rule 34: bool_expr -> expr > expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_gt->GetId());
			},

			// rule 35: bool_expr -> expr < expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_lt->GetId());
			},

			// rule 36: bool_expr -> expr >= expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_gequ->GetId());
			},

			// rule 37: bool_expr -> expr <= expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_lequ->GetId());
			},

			// rule 38: bool_expr -> expr == expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_equ->GetId());
			},

			// rule 39: bool_expr -> expr != expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = bool_expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_nequ->GetId());
			},

			// rule 40: idents -> ident, idents
			[](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				auto ident = std::dynamic_pointer_cast<ASTToken<std::string>>(args[0]);
				ident->SetIdent(true);
				auto idents_lst = std::dynamic_pointer_cast<ASTList>(args[2]);

				idents_lst->AddChild(ident, true);
				return idents_lst;
			},

			// rule 41: idents -> ident
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = idents->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				auto idents_lst = std::make_shared<ASTList>(id, tableidx);

				auto ident = std::dynamic_pointer_cast<ASTToken<std::string>>(args[0]);
				ident->SetIdent(true);

				idents_lst->AddChild(ident, true);
				return idents_lst;
			},

			// rule 42, idents -> eps
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = idents->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTList>(id, tableidx);
			},

			// rule 43: exprs -> expr, exprs
			[](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				auto exprs_lst = std::dynamic_pointer_cast<ASTList>(args[2]);
				exprs_lst->AddChild(args[0], false);
				return exprs_lst;
			},

			// rule 44: exprs -> expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = exprs->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				auto exprs_lst = std::make_shared<ASTList>(id, tableidx);

				exprs_lst->AddChild(args[0], false);
				return exprs_lst;
			},

			// rule 45, exprs -> eps
			[&mapNonTermIdx]([[maybe_unused]] const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = exprs->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTList>(id, tableidx);
			},

			// rule 46, binary not: expr -> ~expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTUnary>(
					id, tableidx, args[1], op_binnot->GetId());
			},

			// rule 47: expr -> expr bin_and expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_binand->GetId());
			},

			// rule 48: expr -> expr bin_or expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_binor->GetId());
			},

			// rule 49: expr -> expr bin_xor expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_binxor->GetId());
			},

			// rule 50: expr -> expr << expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_shift_left->GetId());
			},

			// rule 51: expr -> expr >> expr
			[&mapNonTermIdx](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = expr->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(
					id, tableidx, args[0], args[2], op_shift_right->GetId());
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
					return std::make_tuple(false, "");
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
			std::cout << "\nTokens: ";
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
			ast->AssignLineNumbers();
			ast->DeriveDataType();

#if DEBUG_CODEGEN != 0
			std::cout << "\nAST:\n";
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

				std::make_pair('&', std::make_tuple("binand", OpCode::BINAND)),
				std::make_pair('|', std::make_tuple("binand", OpCode::BINOR)),
				std::make_pair('~', std::make_tuple("binand", OpCode::BINNOT)),

				std::make_pair('>', std::make_tuple("gt", OpCode::GT)),
				std::make_pair('<', std::make_tuple("lt", OpCode::LT)),
				std::make_pair(static_cast<std::size_t>(Token::EQU),
					std::make_tuple("equ", OpCode::EQU)),
				std::make_pair(static_cast<std::size_t>(Token::NEQU),
					std::make_tuple("nequ", OpCode::NEQU)),
				std::make_pair(static_cast<std::size_t>(Token::GEQU),
					std::make_tuple("gequ", OpCode::GEQU)),
				std::make_pair(static_cast<std::size_t>(Token::LEQU),
					std::make_tuple("lequ", OpCode::LEQU)),
				std::make_pair(static_cast<std::size_t>(Token::AND),
					std::make_tuple("and", OpCode::AND)),
				std::make_pair(static_cast<std::size_t>(Token::OR),
					std::make_tuple("or", OpCode::OR)),

				std::make_pair(static_cast<std::size_t>(Token::BIN_XOR),
					std::make_tuple("binxor", OpCode::BINXOR)),
				std::make_pair(static_cast<std::size_t>(Token::SHIFT_LEFT),
					std::make_tuple("shl", OpCode::SHL)),
				std::make_pair(static_cast<std::size_t>(Token::SHIFT_RIGHT),
					std::make_tuple("shr", OpCode::SHR)),
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
			astasmbin.PatchFunctionAddresses();
			astasmbin.FinishCodegen();
			std::string strAsmBin = ostrAsmBin.str();

#if DEBUG_CODEGEN != 0
			std::cout << "\nSymbol table:\n";
			std::cout << astasmbin.GetSymbolTable();

			std::cout << "\nGenerated code ("
				<< strAsmBin.size() << " bytes):\n"
				<< ostrAsm.str();
#endif

			fs::path binfile(script_file ? script_file : "script.scr");
			binfile = binfile.filename();
			binfile.replace_extension(".bin");

			std::ofstream ofstrAsmBin(binfile.string(), std::ios_base::binary);
			if(!ofstrAsmBin)
			{
				std::cerr << "Cannot open \""
					<< binfile.string() << "\"." << std::endl;
				return std::make_tuple(false, "");
			}
			ofstrAsmBin.write(strAsmBin.data(), strAsmBin.size());
			if(ofstrAsmBin.fail())
			{
				std::cerr << "Cannot write \""
					<< binfile.string() << "\"." << std::endl;
				return std::make_tuple(false, "");
			}
			ofstrAsmBin.flush();

			std::cout << "\nCreated compiled program \""
				<< binfile.string() << "\"." << std::endl;

			return std::make_tuple(true, strAsmBin);
		}
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return std::make_tuple(false, "");
	}

	return std::make_tuple(true, "");
}

#endif
#endif  // RUN_PARSER



#if defined(RUN_PARSER) && RUN_VM != 0
static bool run_vm(const std::string& prog)
{
	VM vm(4096);
	//vm.SetDebug(true);
	//vm.SetZeroPoppedVals(true);
	//vm.SetDrawMemImages(true);
	VM::t_addr sp_initial = vm.GetSP();
	vm.SetMem(0, prog, true);
	vm.Run();

	if(vm.GetSP() != sp_initial)
	{
		std::cout << "\nResult: ";
		std::visit([](auto&& val) -> void
		{
			using t_val = std::decay_t<decltype(val)>;
			if constexpr(!std::is_same_v<t_val, std::monostate>)
				std::cout << val;
			else
				std::cout << "<none>";
		}, vm.TopData());
		std::cout << std::endl;
	}

	return true;
}
#endif



int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	std::ios_base::sync_with_stdio(false);

#ifdef CREATE_PARSER
	t_timepoint start_parsergen = t_clock::now();

	create_symbols();
	create_grammar();
	if(lr1_create_parser())
	{
		t_duration time_parsergen = t_clock::now() - start_parsergen;
		std::cout << "Parser generation time: " << time_parsergen.count() << " s." << std::endl;
	}
#endif

#ifdef RUN_PARSER
	t_timepoint start_codegen = t_clock::now();
	const char* script_file = nullptr;
	if(argc >= 2)
		script_file = argv[1];

	create_symbols();
	if(auto [code_ok, prog] = lr1_run_parser(script_file); code_ok)
	{
		t_duration time_codegen = t_clock::now() - start_codegen;
		std::cout << "Code generation time: " << time_codegen.count() << " s." << std::endl;

#if RUN_VM != 0
		t_timepoint start_vm = t_clock::now();
		if(run_vm(prog))
		{
			t_duration time_vm = t_clock::now() - start_vm;
			std::cout << "VM execution time: " << time_vm.count() << " s." << std::endl;
		}
#endif
	}
#endif

	return 0;
}
