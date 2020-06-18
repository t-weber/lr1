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
	POW_TERM,
	FACTOR
};


int main()
{
	try
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
		auto comma = std::make_shared<Terminal>(',', ",");
		auto sym = std::make_shared<Terminal>((std::size_t)Token::REAL, "symbol");
		auto ident = std::make_shared<Terminal>((std::size_t)Token::IDENT, "ident");


		std::size_t semanticindex = 0;

		// rule 0
		start->AddRule({ add_term }, semanticindex++);
		// rule 1
		add_term->AddRule({ add_term, plus, mul_term }, semanticindex++);
		// rule 2
		add_term->AddRule({ add_term, minus, mul_term }, semanticindex++);
		// rule 3
		add_term->AddRule({ mul_term }, semanticindex++);
		// rule 4
		mul_term->AddRule({ mul_term, mult, pow_term }, semanticindex++);
		// rule 5
		mul_term->AddRule({ mul_term, div, pow_term }, semanticindex++);
		// rule 6
		mul_term->AddRule({ mul_term, mod, pow_term }, semanticindex++);
		// rule 7
		mul_term->AddRule({ pow_term }, semanticindex++);
		// rule 8
		pow_term->AddRule({ pow_term, pow, factor }, semanticindex++);
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


		std::vector<NonTerminalPtr> all_nonterminals{{start, add_term, mul_term, pow_term, factor}};

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
		std::cout << "\n\nLR(1):\n" << colls << std::endl;

		Collection collsLALR = colls.ConvertToLALR();
		collsLALR.WriteGraph("expr_lalr_full", 1);
		std::cout << "\n\nLALR(1):\n" << collsLALR << std::endl;


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
			[&mapNonTermIdx, &add_term, &minus](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = add_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], minus->GetId());
			},

			// rule 3
			[&mapNonTermIdx, &add_term](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = add_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 4
			[&mapNonTermIdx, &mul_term, &mult](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], mult->GetId());
			},

			// rule 5
			[&mapNonTermIdx, &mul_term, &div](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], div->GetId());
			},

			// rule 6
			[&mapNonTermIdx, &mul_term, &mod](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], mod->GetId());
			},

			// rule 7
			[&mapNonTermIdx, &mul_term](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = mul_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 8
			[&mapNonTermIdx, &pow_term, &pow](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = pow_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTBinary>(id, tableidx, args[0], args[2], pow->GetId());
			},

			// rule 9
			[&mapNonTermIdx, &pow_term](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = pow_term->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 10
			[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = factor->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[1]);
			},

			// rule 11
			[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 12
			[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 13
			[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				// TODO: function call
				std::cerr << "not yet implemented" << std::endl;
				return nullptr;
			},

			// rule 14
			[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
			{
				std::size_t id = factor->GetId();
				std::size_t tableidx = mapNonTermIdx.find(id)->second;
				return std::make_shared<ASTDelegate>(id, tableidx, args[0]);
			},

			// rule 15
			[&mapNonTermIdx, &factor](const std::vector<t_astbaseptr>& args) -> t_astbaseptr
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
				std::cout << tok->GetId() << " ";
			std::cout << "\n";

			auto ast = cst_to_ast(parser.Parse(tokens));
			std::cout << "AST:\n";
			ast->print(std::cout);
		}
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return -1;
	}

	return 0;
}
