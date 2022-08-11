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

#include "parsergen.h"

#include <fstream>
#include <sstream>

#include <boost/algorithm/string.hpp>


ParserGen::ParserGen(
	const std::tuple<
		t_table,                 // 0: shift table
		t_table,                 // 1: reduce table
		t_table,                 // 2: jump table
		t_mapIdIdx,              // 3: terminal indices
		t_mapIdIdx,              // 4: nonterminal indices
		t_vecIdx>& init)         // 5: semantic rule indices
	: m_tabActionShift{std::get<0>(init)},
		m_tabActionReduce{std::get<1>(init)},
		m_tabJump{std::get<2>(init)},
		m_mapTermIdx{std::get<3>(init)},
		m_mapNonTermIdx{std::get<4>(init)},
		m_numRhsSymsPerRule{std::get<5>(init)}
{}


ParserGen::ParserGen(
	const std::tuple<
		const t_table*,          // 0: shift table
		const t_table*,          // 1: reduce table
		const t_table*,          // 2: jump table
		const t_mapIdIdx*,       // 3: terminal indices
		const t_mapIdIdx*,       // 4: nonterminal indices
		const t_vecIdx*>& init)  // 5: semantic rule indices
	: m_tabActionShift{*std::get<0>(init)},
		m_tabActionReduce{*std::get<1>(init)},
		m_tabJump{*std::get<2>(init)},
		m_mapTermIdx{*std::get<3>(init)},
		m_mapNonTermIdx{*std::get<4>(init)},
		m_numRhsSymsPerRule{*std::get<5>(init)}
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


bool ParserGen::CreateParser(const std::string& filename_cpp) const
{
	// output header file stub
	std::string outfile_h = R"raw(
#ifndef __LR1_PARSER_REC_ASC_H__
#define __LR1_PARSER_REC_ASC_H__

#include "src/codegen/ast.h"
#include "src/parsergen/common.h"

#include <stack>

class ParserRecAsc
{
public:
	ParserRecAsc(const std::vector<t_semanticrule>* rules);
	ParserRecAsc() = delete;
	ParserRecAsc(const ParserRecAsc&) = delete;
	ParserRecAsc& operator=(const ParserRecAsc&) = delete;

	t_astbaseptr Parse(const std::vector<t_toknode>* input);

protected:
	void PrintSymbols();
	void GetNextLookahead();

%%DECLARE_CLOSURES%%

private:
	// semantic rules
	const std::vector<t_semanticrule>* m_semantics{nullptr};

	// input tokens
	const std::vector<t_toknode>* m_input{nullptr};

	// lookahead token
	t_toknode m_lookahead{nullptr};

	// lookahead table index
	std::size_t m_lookahead_tabidx{0};

	// index into input token array
	int m_lookahead_idx{-1};

	// currently active symbols
	std::stack<t_astbaseptr> m_symbols{};

	// number of function returns between reduction and performing jump / non-terminal transition
	std::size_t m_dist_to_jump{0};

	// input was accepted
	bool m_accepted{false};
};

#endif
)raw";


	// output cpp file stub
	std::string outfile_cpp = R"raw(
%%INCLUDE_HEADER%%

ParserRecAsc::ParserRecAsc(const std::vector<t_semanticrule>* rules)
	: m_semantics{rules}
{}

void ParserRecAsc::PrintSymbols()
{
	std::stack<t_astbaseptr> symbols = m_symbols;

	std::cout << symbols.size() << " symbols: ";

	while(symbols.size())
	{
		t_astbaseptr sym = symbols.top();
		symbols.pop();

		std::cout << sym->GetId() << " (" << sym->GetTableIdx() << "), ";
	}
	std::cout << std::endl;
}

void ParserRecAsc::GetNextLookahead()
{
	++m_lookahead_idx;
	if(m_lookahead_idx >= int(m_input->size()) || m_lookahead_idx < 0)
	{
		m_lookahead = nullptr;
		m_lookahead_tabidx = 0;
	}
	else
	{
		m_lookahead = (*m_input)[m_lookahead_idx];
		m_lookahead_tabidx = m_lookahead->GetTableIdx();
	}
}

t_astbaseptr ParserRecAsc::Parse(const std::vector<t_toknode>* input)
{
	m_input = input;
	m_lookahead_idx = -1;
	m_lookahead_tabidx = 0;
	m_lookahead = nullptr;
	m_dist_to_jump = 0;
	m_accepted = false;
	while(!m_symbols.empty())
		m_symbols.pop();

	GetNextLookahead();
	closure_0();

	if(m_symbols.size() && m_accepted)
		return m_symbols.top();
	return nullptr;
}

%%DEFINE_CLOSURES%%
)raw";


	std::string filename_h = filename_cpp + ".h";


	// open output files
	std::ofstream file_cpp(filename_cpp);
	std::ofstream file_h(filename_h);

	if(!file_cpp || !file_h)
	{
		std::cerr << "Cannot open output files \"" << filename_cpp
			<< "\" and \"" << filename_h << "\"." << std::endl;
		return false;
	}


	// generate closures
	std::ostringstream ostr_h, ostr_cpp;
	std::size_t num_closures = m_tabActionShift.size1();

	for(std::size_t closure_idx=0; closure_idx<num_closures; ++closure_idx)
	{
		ostr_h << "\tvoid closure_" << closure_idx << "();\n";

		ostr_cpp << "void ParserRecAsc::closure_" << closure_idx << "()\n";
		ostr_cpp << "{\n";

		if(m_generate_debug)
		{
			ostr_cpp << "\tstd::cout << \"\\nRunning \" << __PRETTY_FUNCTION__ << \"...\" << std::endl;\n";
			ostr_cpp << "\tif(m_lookahead)\n";
			ostr_cpp << "\t\tstd::cout << \"Lookahead [\"  << m_lookahead_idx << \"]: \""
				" << m_lookahead->GetId() << \" (\" << m_lookahead->GetTableIdx() << \")\" << std::endl;\n";
			ostr_cpp << "\tPrintSymbols();\n";
		}

		// shift actions == shift action table entries == terminal transitions between closures
		bool first_alternative = true;
		for(std::size_t tok_idx=0; tok_idx<m_tabActionShift.size2(); ++tok_idx)
		{
			if(std::size_t newclosure = m_tabActionShift(closure_idx, tok_idx);
			   newclosure != ERROR_VAL)
			{
				ostr_cpp << "\t";
				if(!first_alternative)
					ostr_cpp << "else ";
				ostr_cpp << "if(m_lookahead_tabidx == " << tok_idx << ")\n";
				ostr_cpp << "\t{\n";
				ostr_cpp << "\t\tm_symbols.push(m_lookahead);\n";
				ostr_cpp << "\t\tGetNextLookahead();\n";
				ostr_cpp << "\t\tclosure_" << newclosure << "();\n";
				ostr_cpp << "\t}\n";

				first_alternative = false;
			}
		}

		// reduce actions == reduce action table entries == closures with cursor at end
		for(std::size_t tok_idx=0; tok_idx<m_tabActionReduce.size2(); ++tok_idx)
		{
			if(std::size_t newrule = m_tabActionReduce(closure_idx, tok_idx);
			   newrule != ERROR_VAL)
			{
				ostr_cpp << "\t";
				if(!first_alternative)
					ostr_cpp << "else ";

				ostr_cpp << "if(m_lookahead_tabidx == " << tok_idx << ")\n";
				ostr_cpp << "\t{\n";

				if(newrule == ACCEPT_VAL)
				{
					ostr_cpp << "\t\tm_accepted = true;\n";
				}
				else
				{
					std::size_t num_rhs = m_numRhsSymsPerRule[newrule];
					ostr_cpp << "\t\tm_dist_to_jump = " << num_rhs << ";\n";

					if(m_generate_debug)
					{
						ostr_cpp << "\t\tstd::cout << \"Reducing " << num_rhs
							<< " symbols using rule " << newrule << ".\" << std::endl;\n";
					}

					// take the symbols from the stack and create an argument vector for the semantic rule
					ostr_cpp << "\t\tstd::vector<t_astbaseptr> args;\n";
					for(std::size_t arg=0; arg<num_rhs; ++arg)
					{
						ostr_cpp << "\t\targs.insert(args.begin(), m_symbols.top());\n";
						ostr_cpp << "\t\tm_symbols.pop();\n";
					}

					// execute semantic rule
					ostr_cpp << "\t\tt_astbaseptr reducedSym = (*m_semantics)[" << newrule << "](args);\n";
					ostr_cpp << "\t\tm_symbols.push(reducedSym);\n";
				}

				ostr_cpp << "\t}\n";  // end if
				first_alternative = false;
			}
		}

		// jump to new closure == jump table entries == non-terminal transitions between closures
		std::ostringstream ostr_cpp_while;
		ostr_cpp_while << "\twhile(!m_dist_to_jump && m_symbols.size() && !m_accepted)\n";
		ostr_cpp_while << "\t{\n";

		ostr_cpp_while << "\t\tt_astbaseptr topsym = m_symbols.top();\n";
		ostr_cpp_while << "\t\tif(topsym->IsTerminal())\n\t\t\tbreak;\n";
		ostr_cpp_while << "\t\tstd::size_t topsym_tabidx = topsym->GetTableIdx();\n";

		bool while_has_entries = false;
		first_alternative = true;
		for(std::size_t nonterm_idx=0; nonterm_idx<m_tabJump.size2(); ++nonterm_idx)
		{
			if(std::size_t newclosure = m_tabJump(closure_idx, nonterm_idx);
				newclosure != ERROR_VAL)
			{
				ostr_cpp_while << "\t\t";
				if(!first_alternative)
					ostr_cpp_while << "else ";
				ostr_cpp_while << "if(topsym_tabidx == " << nonterm_idx << ")\n";
				ostr_cpp_while << "\t\t{\n";
				ostr_cpp_while << "\t\t\tclosure_" << newclosure << "();\n";
				ostr_cpp_while << "\t\t}\n";

				first_alternative = false;
				while_has_entries = true;
			}
		}

		ostr_cpp_while << "\t}\n";  // end while
		if(while_has_entries)
			ostr_cpp << ostr_cpp_while.str();

		// return from closure -> decrement distance counter
		ostr_cpp << "\tif(m_dist_to_jump > 0)\n\t\t--m_dist_to_jump;\n";

		if(m_generate_debug)
		{
			ostr_cpp << "\tstd::cout << \"Returning from closure, distance to jump: \" << m_dist_to_jump << \".\" << std::endl;\n";
		}
		ostr_cpp << "}\n\n";  // end closure
	}


	// write output files
	std::string incl = "#include \"" + filename_h + "\"";
	boost::replace_all(outfile_cpp, "%%INCLUDE_HEADER%%", incl);
	boost::replace_all(outfile_cpp, "%%DEFINE_CLOSURES%%", ostr_cpp.str());
	boost::replace_all(outfile_h, "%%DECLARE_CLOSURES%%", ostr_h.str());

	file_cpp << outfile_cpp << std::endl;
	file_h << outfile_h << std::endl;

	return true;
}
