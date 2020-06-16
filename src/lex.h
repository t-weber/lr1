/**
 * simple lexer
 * @author Tobias Weber
 * @date 7-mar-20
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LEX_H__
#define __LEX_H__

#include <iostream>
#include <vector>
#include <map>
#include <utility>
#include <optional>
#include <variant>

#include "ast.h"
#include "common.h"


using t_real = double;
using t_lval = std::optional<std::variant<t_real>>;
using t_tok = std::size_t;


enum class Token : t_tok
{
	REAL	= 1000,
	IDENT	= 1001,

	END		= END_IDENT,
};


/**
 * find all matching tokens for input string
 */
extern std::vector<std::tuple<t_tok, t_lval>> get_matching_tokens(const std::string& str);


/**
 * get next token and attribute
 */
extern std::tuple<t_tok, t_lval> get_next_token(std::istream& istr = std::cin);


/**
 * get all tokens and attributes
 */
extern std::vector<t_toknode> get_all_tokens(
	std::istream& istr = std::cin, const t_mapIdIdx* mapTermIdx = nullptr);


#endif
