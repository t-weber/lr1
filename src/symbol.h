/**
 * symbols
 * @author Tobias Weber
 * @date 07-jun-2020
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __SYM_H__
#define __SYM_H__


#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <optional>
#include <iostream>


class Symbol;
class Terminal;
class NonTerminal;
class Word;

using SymbolPtr = std::shared_ptr<Symbol>;
using TerminalPtr = std::shared_ptr<Terminal>;
using NonTerminalPtr = std::shared_ptr<NonTerminal>;
using WordPtr = std::shared_ptr<Word>;


/**
 * symbol base class
 */
class Symbol
{
public:
	Symbol(std::size_t id, const std::string& strid="", bool bEps=false, bool bEnd=false);
	Symbol() = delete;
	virtual ~Symbol() = default;

	virtual bool IsTerminal() const = 0;

	const std::string& GetStrId() const { return m_strid; }
	std::size_t GetId() const { return m_id; }

	bool IsEps() const { return m_iseps; }
	bool IsEnd() const { return m_isend; }

	bool operator==(const Symbol& other) const;
	bool operator!=(const Symbol& other) const { return !operator==(other); }

	virtual void print(std::ostream& ostr) const = 0;
	virtual std::size_t hash() const = 0;


private:
	std::size_t m_id = 0;
	std::string m_strid;
	std::size_t m_idx = 0;

	bool m_iseps = false;
	bool m_isend = false;
};



/**
 * terminal symbols
 */
class Terminal : public Symbol
{
public:
	Terminal(std::size_t id, const std::string& strid="", bool bEps=false, bool bEnd=false)
		: Symbol{id, strid, bEps, bEnd}, m_semantic{} {}
	Terminal() = delete;
	virtual ~Terminal() = default;

	virtual bool IsTerminal() const override { return 1; }

	/**
	 * get the semantic rule index
	 */
	std::optional<std::size_t> GetSemanticRule() const { return m_semantic; }

	/**
	 * set the semantic rule index
	 */
	void SetSemanticRule(std::optional<std::size_t> semantic=std::nullopt){ m_semantic = semantic; }

	virtual void print(std::ostream& ostr) const override;
	virtual std::size_t hash() const override;


public:
	static std::function<bool(const TerminalPtr term1, const TerminalPtr term2)> terminals_compare;
	using t_terminalset = std::set<TerminalPtr, decltype(terminals_compare)>;


private:
	// semantic rule
	std::optional<std::size_t> m_semantic;
};



/**
 * nonterminal symbols
 */
class NonTerminal : public Symbol
{
public:
	NonTerminal(std::size_t id, const std::string& strid="")
		: Symbol{id, strid}, m_rules{}, m_semantics{} {}
	NonTerminal() = delete;
	virtual ~NonTerminal() = default;

	virtual bool IsTerminal() const override { return 0; }

	/**
	 * add multiple alternative production rules
	 */
	void AddRule(const Word& rule, std::optional<std::size_t> semanticruleidx=std::nullopt)
	{
		m_rules.push_back(rule);
		m_semantics.push_back(semanticruleidx);
	}

	/**
	 * number of rules
	 */
	std::size_t NumRules() const { return m_rules.size(); }

	/**
	 * get a production rule
	 */
	const Word& GetRule(std::size_t i) const { return m_rules[i]; }

	/**
	 * get a semantic rule index
	 */
	std::optional<std::size_t> GetSemanticRule(std::size_t i) const { return m_semantics[i]; }

	/**
	 * does this non-terminal have a rule which produces epsilon?
	 */
	bool HasEpsRule() const;

	virtual void print(std::ostream& ostr) const override;
	virtual std::size_t hash() const override;


private:
	// production syntactic rules
	std::vector<Word> m_rules;

	// production semantic rule indices
	std::vector<std::optional<std::size_t>> m_semantics;
};



/**
 * a collection of terminals and non-terminals
 */
class Word
{
public:
	Word(const std::initializer_list<SymbolPtr> init) : m_syms{init} {}
	Word(const Word& other) : m_syms{other.m_syms} {}
	Word() : m_syms{} {}

	/**
	 * add a symbol to the word
	 */
	void AddSymbol(SymbolPtr sym)
	{
		m_syms.push_back(sym);
	}

	/**
	 * number of symbols in the word
	 */
	std::size_t NumSymbols() const { return m_syms.size(); }
	std::size_t size() const { return NumSymbols(); }

	/**
	 * get a symbol in the word
	 */
	const SymbolPtr GetSymbol(const std::size_t i) const { return m_syms[i]; }
	const SymbolPtr operator[](const std::size_t i) const { return GetSymbol(i); }

	/**
	 * test for equality
	 */
	bool operator==(const Word& other) const;
	bool operator!=(const Word& other) const { return !operator==(other); }

	std::size_t hash() const;

	friend std::ostream& operator<<(std::ostream& ostr, const Word& word);


private:
	std::vector<SymbolPtr> m_syms;
};



// ----------------------------------------------------------------------------


/**
 * epsilon and end symbols
 */
extern const TerminalPtr g_eps, g_end;


// ----------------------------------------------------------------------------



/**
 * calculates the first set of a nonterminal
 */
extern void calc_first(const NonTerminalPtr nonterm,
	std::map<std::string, Terminal::t_terminalset>& _first,
	std::map<std::string, std::vector<Terminal::t_terminalset>>* _first_per_rule=nullptr);


/**
 * calculates the follow set of a nonterminal
 */
extern void calc_follow(const std::vector<NonTerminalPtr>& allnonterms,
	const NonTerminalPtr& start, const NonTerminalPtr nonterm,
	const std::map<std::string, Terminal::t_terminalset>&  _first,
	std::map<std::string, Terminal::t_terminalset>& _follow);


#endif
