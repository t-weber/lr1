/**
 * lr(1) parser
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 15-jun-2020
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 * 	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 * 	- "Übersetzerbau", ISBN: 978-3540653899 (1999, 2013)
 */

#ifndef __LR1_PARSER_H__
#define __LR1_PARSER_H__

#include "ast.h"
#include "../parsergen/common.h"


/**
 * lr(1) parser
 */
class Parser
{
public:
	// directly takes the input from Collection::CreateParseTables
	Parser(const std::tuple<
		t_table, t_table, t_table,
		t_mapIdIdx, t_mapIdIdx, t_vecIdx>& init,
		const std::vector<t_semanticrule>& rules);

	Parser(const std::tuple<const t_table*, const t_table*, const t_table*,
		   const t_mapIdIdx*, const t_mapIdIdx*, const t_vecIdx*>& init,
		   const std::vector<t_semanticrule>& rules);

	Parser() = delete;

	const t_mapIdIdx& GetTermIndexMap() const { return m_mapTermIdx; }

	t_astbaseptr Parse(const std::vector<t_toknode>& input) const;


private:
	// parse tables
	t_table m_tabActionShift{};
	t_table m_tabActionReduce{};
	t_table m_tabJump{};

	// mappings from symbol id to table index
	t_mapIdIdx m_mapTermIdx{};
	t_mapIdIdx m_mapNonTermIdx{};

	// number of symbols on right hand side of a rule
	t_vecIdx m_numRhsSymsPerRule{};

	// semantic rules
	std::vector<t_semanticrule> m_semantics{};
};


#endif
