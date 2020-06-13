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
	Symbol(const std::string& id, bool bEps=false, bool bEnd=false)
		: m_id{id}, m_iseps{bEps}, m_isend{bEnd} {}
	Symbol() = delete;
	virtual ~Symbol() = default;

	virtual bool IsTerminal() const = 0;
	const std::string& GetId() const { return m_id; }

	bool IsEps() const { return m_iseps; }
	bool IsEnd() const { return m_isend; }

	bool operator==(const Symbol& other) const;
	bool operator!=(const Symbol& other) const { return !operator==(other); }

	virtual void print(std::ostream& ostr) const = 0;
	virtual std::size_t hash() const = 0;


private:
	std::string m_id;
	bool m_iseps = false;
	bool m_isend = false;
};



/**
 * terminal symbols
 */
class Terminal : public Symbol
{
public:
	Terminal(const std::string& id, bool bEps=false, bool bEnd=false) : Symbol{id, bEps, bEnd} {}
	Terminal() = delete;
	virtual ~Terminal() = default;

	virtual bool IsTerminal() const override { return 1; }

	virtual void print(std::ostream& ostr) const override;
	virtual std::size_t hash() const override;

public:
	static std::function<bool(const TerminalPtr term1, const TerminalPtr term2)> terminals_compare;
	using t_terminalset = std::set<TerminalPtr, decltype(terminals_compare)>;
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

	void AddSymbol(SymbolPtr sym)
	{
		m_syms.push_back(sym);
	}

	std::size_t NumSymbols() const { return m_syms.size(); }
	std::size_t size() const { return NumSymbols(); }

	const SymbolPtr GetSymbol(const std::size_t i) const { return m_syms[i]; }
	const SymbolPtr operator[](const std::size_t i) const { return GetSymbol(i); }

	bool operator==(const Word& other) const;
	bool operator!=(const Word& other) const { return !operator==(other); }

	std::size_t hash() const;

	friend std::ostream& operator<<(std::ostream& ostr, const Word& word);


private:
	std::vector<SymbolPtr> m_syms;
};



/**
 * nonterminal symbols
 */
class NonTerminal : public Symbol
{
public:
	NonTerminal(const std::string& id) : Symbol{id}, m_rules{} {}
	NonTerminal() = delete;
	virtual ~NonTerminal() = default;

	virtual bool IsTerminal() const override { return 0; }

	/**
	 * add multiple alternative production rules
	 */
	void AddRule(const Word& rule) { m_rules.push_back(rule); }

	/**
	 * number of rules
	 */
	std::size_t NumRules() const { return m_rules.size(); }

	/**
	 * get a production rule
	 */
	const Word& GetRule(std::size_t i) const { return m_rules[i]; }

	/**
	 * does this non-terminal have a rule which produces epsilon?
	 */
	bool HasEpsRule() const;

	virtual void print(std::ostream& ostr) const override;
	virtual std::size_t hash() const override;


private:
	// production rules
	std::vector<Word> m_rules;
};



// ----------------------------------------------------------------------------


extern TerminalPtr g_eps;
extern TerminalPtr g_end;


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
