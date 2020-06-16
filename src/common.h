/**
 * common types
 * @author Tobias Weber
 * @date 15-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_TYPES_H__
#define __LR1_TYPES_H__

#include "helpers.h"
#include "ast.h"

#include <vector>
#include <map>


#define ERROR_VAL 0xffffffff	/* 'error' table entry */
#define ACCEPT_VAL 0xfffffffe	/* 'accept' table entry */

#define EPS_IDENT 0xffffff00	/* epsilon token id*/
#define END_IDENT 0xffffff01	/* end token id*/


using t_toknode = t_astbaseptr;

using t_table = Table<std::size_t, std::vector>;
using t_mapIdIdx = std::map<std::size_t, std::size_t>;
using t_vecIdx = std::vector<std::size_t>;


#endif
