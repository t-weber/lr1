/**
 * simple lexer
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 7-mar-20
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_LEXER_H__
#define __LR1_LEXER_H__

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <optional>

#include "ast.h"
#include "lval.h"
#include "../parsergen/common.h"


using t_tok = std::size_t;


enum class Token : t_tok
{
	REAL        = 1000,
	INT         = 1001,
	STR         = 1002,
	IDENT       = 1003,

	EQU         = 2000,
	NEQU        = 2001,
	GEQU        = 2002,
	LEQU        = 2003,

	// logical operators
	AND         = 3000,
	OR          = 3001,

	// binary operators
	BIN_XOR     = 3100,

	IF          = 4000,
	ELSE        = 4001,

	LOOP        = 5000,
	BREAK       = 5001,
	CONTINUE    = 5002,

	FUNC        = 6000,
	RETURN      = 6001,
	EXTERN      = 6002,

	SHIFT_LEFT  = 7000,
	SHIFT_RIGHT = 7001,

	END         = END_IDENT,
};


/**
 * find all matching tokens for input string
 */
extern std::vector<std::tuple<t_tok, t_lval>> get_matching_tokens(
	const std::string& str);


/**
 * get next token and attribute
 */
extern std::tuple<t_tok, t_lval> get_next_token(
	std::istream& istr = std::cin, bool end_on_newline = true);


/**
 * get all tokens and attributes
 */
extern std::vector<t_toknode> get_all_tokens(
	std::istream& istr = std::cin, const t_mapIdIdx* mapTermIdx = nullptr,
	bool end_on_newline = true);


#endif
