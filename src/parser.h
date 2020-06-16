/**
 * lr(1) parser
 * @author Tobias Weber
 * @date 15-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_PARSER_H__
#define __LR1_PARSER_H__

#include "ast.h"
#include "common.h"


/**
 * lr(1) parser
 */
class Parser
{
public:
	// directly takes the input from Collection::CreateParseTables
	Parser(const std::tuple<t_table, t_table, t_table, t_mapIdIdx, t_mapIdIdx, t_vecIdx>& init,
		   const std::vector<t_semanticrule>& rules);
	Parser() = delete;

	const t_mapIdIdx& GetTermIndexMap() const { return m_mapTermIdx; }

	void SetInput(const std::vector<t_toknode>& input) { m_input = input; }
	void Parse() const;


private:
	// parse tables
	t_table m_tabActionShift;
	t_table m_tabActionReduce;
	t_table m_tabJump;

	// mappings from symbol id to table index
	t_mapIdIdx m_mapTermIdx;
	t_mapIdIdx m_mapNonTermIdx;

	// number of symbols on right hand side of a rule
	t_vecIdx m_numRhsSymsPerRule;

	// semantic rules
	std::vector<t_semanticrule> m_semantics;

	// input tokens
	std::vector<t_toknode> m_input;
};


#endif
