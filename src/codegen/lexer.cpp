/**
 * simple lexer
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 7-mar-20
 * @license see 'LICENSE.EUPL' file
 */

#include "lexer.h"

#include <sstream>
#include <memory>
#include <regex>


/**
 * find all matching tokens for input string
 */
std::vector<std::tuple<t_tok, t_lval>>
get_matching_tokens(const std::string& str)
{
	std::vector<std::tuple<t_tok, t_lval>> matches;

	{	// real
		std::regex regex{"[0-9]+(\\.[0-9]*)?"};
		std::smatch smatch;
		if(std::regex_match(str, smatch, regex))
		{
			t_real val{};
			std::istringstream{str} >> val;
			matches.push_back(std::make_tuple((t_tok)Token::REAL, val));
		}
	}

	{	// ident
		std::regex regex{"[A-Za-z]+[A-Za-z0-9]*"};
		std::smatch smatch;
		if(std::regex_match(str, smatch, regex))
			matches.push_back(std::make_tuple((t_tok)Token::IDENT, str));
	}

	{	// tokens represented by themselves
		if(str == "+" || str == "-" || str == "*" || str == "/" ||
			str == "%" || str == "^" || str == "(" || str == ")" ||
			str == "," || str == ";")
			matches.push_back(std::make_tuple((t_tok)str[0], std::nullopt));
	}

	//std::cerr << "Input \"" << str << "\" has " << matches.size() << " matches." << std::endl;
	return matches;
}


/**
 * get next token and attribute
 */
std::tuple<t_tok, t_lval> get_next_token(std::istream& istr)
{
	std::string input;
	std::vector<std::tuple<t_tok, t_lval>> longest_matching;
	bool eof = false;

	// find longest matching token
	while(1)
	{
		eof = istr.eof();
		if(eof)
			break;

		char c = istr.get();

		// if outside any other match...
		if(longest_matching.size() == 0)
		{
			// ...ignore white spaces
			if(c==' ' || c=='\t')
				continue;
			// ...end on new line
			if(c=='\n')
				return std::make_tuple((t_tok)Token::END, std::nullopt);
		}

		input += c;
		auto matching = get_matching_tokens(input);
		if(matching.size())
		{
			longest_matching = matching;

			if(istr.peek() == std::char_traits<char>::eof())
				break;
		}
		else
		{
			// no more matches
			istr.putback(c);
			break;
		}
	}

	if(longest_matching.size() == 0 && eof)
		return std::make_tuple((t_tok)Token::END, std::nullopt);
	if(longest_matching.size() == 0)
		throw std::runtime_error("Invalid input in lexer: \"" + input + "\".");
	if(longest_matching.size() > 1)
		std::cerr << "Warning: Ambiguous match in lexer for token \"" << input << "\"." << std::endl;

	return longest_matching[0];
}


template<std::size_t IDX> struct _Lval_LoopFunc
{
	void operator()(
		std::vector<t_toknode>* vec, std::size_t id,
		std::size_t tableidx, const t_lval& lval) const
	{
		using t_val = std::variant_alternative_t<IDX, typename t_lval::value_type>;
		if(std::holds_alternative<t_val>(*lval))
			vec->emplace_back(std::make_shared<ASTToken<t_val>>(id, tableidx, std::get<IDX>(*lval)));
	};
};


/**
 * get all tokens and attributes
 */
std::vector<t_toknode> get_all_tokens(
	std::istream& istr, const t_mapIdIdx* mapTermIdx)
{
	std::vector<t_toknode> vec;

	while(1)
	{
		auto tup = get_next_token(istr);
		std::size_t id = std::get<0>(tup);
		const t_lval& lval = std::get<1>(tup);

		// get index into parse tables
		std::size_t tableidx = 0;
		if(mapTermIdx)
		{
			auto iter = mapTermIdx->find(id);
			if(iter != mapTermIdx->end())
				tableidx = iter->second;
		}

		// does this token have an attribute?
		if(lval)
		{
			// find the correct type in the variant
			auto seq = std::make_index_sequence<std::variant_size_v<typename t_lval::value_type>>();
			constexpr_loop<_Lval_LoopFunc>(seq, std::make_tuple(&vec, id, tableidx, lval));
		}
		else
		{
			vec.emplace_back(std::make_shared<ASTToken<void*>>(id, tableidx));
		}

		if(id == (t_tok)Token::END)
			break;
	}

	return vec;
}
