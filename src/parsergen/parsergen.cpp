/**
 * lr(1) parser
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date aug-2022
 * @license see 'LICENSE.EUPL' file
 *
 * References:
 * 	- https://doi.org/10.1016/0020-0190(88)90061-0
 * 	- "Compilerbau Teil 1", ISBN: 3-486-25294-1 (1999)
 * 	- "Übersetzerbau", ISBN: 978-3540653899 (1999, 2013)
 */

#include "parsergen.h"


ParserGen::ParserGen(
	const std::tuple<
		t_table,                 // 0: shift table
		t_table,                 // 1: reduce table
		t_table,                 // 2: jump table
		t_mapIdIdx,              // 3: terminal indices
		t_mapIdIdx,              // 4: nonterminal indices
		t_vecIdx>& init,         // 5: semantic rule indices
	const std::vector<t_semanticrule>& rules)
	: m_tabActionShift{std::get<0>(init)},
		m_tabActionReduce{std::get<1>(init)},
		m_tabJump{std::get<2>(init)},
		m_mapTermIdx{std::get<3>(init)},
		m_mapNonTermIdx{std::get<4>(init)},
		m_numRhsSymsPerRule{std::get<5>(init)},
		m_semantics{rules}
{}


ParserGen::ParserGen(
	const std::tuple<
		const t_table*,          // 0: shift table
		const t_table*,          // 1: reduce table
		const t_table*,          // 2: jump table
		const t_mapIdIdx*,       // 3: terminal indices
		const t_mapIdIdx*,       // 4: nonterminal indices
		const t_vecIdx*>& init,  // 5: semantic rule indices
	const std::vector<t_semanticrule>& rules)
	: m_tabActionShift{*std::get<0>(init)},
		m_tabActionReduce{*std::get<1>(init)},
		m_tabJump{*std::get<2>(init)},
		m_mapTermIdx{*std::get<3>(init)},
		m_mapNonTermIdx{*std::get<4>(init)},
		m_numRhsSymsPerRule{*std::get<5>(init)},
		m_semantics{rules}
{}


template<class t_toknode>
std::string get_line_numbers(const t_toknode& node)
{
	std::ostringstream ostr;

	if(auto lines = node->GetLineRange(); lines)
	{
		auto line_start = std::get<0>(*lines);
		auto line_end = std::get<1>(*lines);

		if(line_start == line_end)
			ostr << " (line " << line_start << ")";
		else
			ostr << " (lines " << line_start << "..." << line_end << ")";
	}

	return ostr.str();
}


bool ParserGen::CreateParser(const std::string& file) const
{
	// TODO

	return true;
}
