/**
 * value type for lexer
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 15-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_LVAL_H__
#define __LR1_LVAL_H__

#include <map>
#include <variant>
#include <string>


using t_real = double;
using t_lval = std::optional<std::variant<t_real, std::string>>;


#endif
