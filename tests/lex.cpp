/**
 * simple lexer
 * @author Tobias Weber
 * @date 7-mar-20
 * @license see 'LICENSE.EUPL' file
 */

#include "lex.h"

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
			matches.push_back(std::make_tuple((t_tok)Token::IDENT, std::nullopt));
	}

	{	// tokens represented by themselves
		if(str == "+" || str == "-" || str == "*" || str == "/" ||
			str == "%" || str == "^" || str == "(" || str == ")" || str == ",")
			matches.push_back(std::make_tuple((t_tok)str[0], std::nullopt));
	}

	/*{	// new line
		if(str == "\n")
			matches.push_back(std::make_tuple((t_tok)Token::END, std::nullopt));
	}*/

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


/**
 * get all tokens and attributes
 */
std::vector<std::tuple<t_tok, t_lval>> get_all_tokens(std::istream& istr)
{
	std::vector<std::tuple<t_tok, t_lval>> vec;

	while(1)
	{
		vec.emplace_back(get_next_token(istr));

		/*std::cout << "token: " << std::get<0>(*vec.rbegin());
		if(std::get<1>(*vec.rbegin()))
			std::cout << ", lvalue: " << std::get<t_real>(*std::get<1>(*vec.rbegin()));
		std::cout << std::endl;*/

		if(std::get<0>(*vec.rbegin()) == (t_tok)Token::END)
			break;
	}

	return vec;
}
