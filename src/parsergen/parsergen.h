/**
 * lr(1) recursive ascent parser generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date aug-2022
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 * 	- https://doi.org/10.1016/0020-0190(88)90061-0
 * 	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 * 	- "Übersetzerbau", ISBN: 978-3540653899 (1999, 2013)
 */

#ifndef __LR1_PARSER_GEN_H__
#define __LR1_PARSER_GEN_H__

#include "../codegen/ast.h"
#include "common.h"


/**
 * lr(1) parser generator
 */
class ParserGen
{
public:
	// directly takes the input from Collection::CreateParseTables
	ParserGen(const std::tuple<
		t_table, t_table, t_table,
		t_mapIdIdx, t_mapIdIdx, t_vecIdx>& init);

	ParserGen(const std::tuple<const t_table*, const t_table*, const t_table*,
		   const t_mapIdIdx*, const t_mapIdIdx*, const t_vecIdx*>& init);

	ParserGen() = delete;

	void SetGenerateDebug(bool b) { m_generate_debug = b; }
	bool CreateParser(const std::string& file) const;


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

	// generate debug messages
	bool m_generate_debug{true};
};


#endif
