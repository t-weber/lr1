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

	bool operator==(const Symbol& other) const
	{
		return this->GetId() == other.GetId();
	}

	bool operator!=(const Symbol& other) const { return !operator==(other); }


private:
	std::string m_id;
	bool m_iseps = false;
	bool m_isend = false;
};


using SymbolPtr = std::shared_ptr<Symbol>;


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
};



/**
 * a collection of terminals and non-terminals
 */
class Word
{
public:
	Word(const std::initializer_list<SymbolPtr> init)
		: m_syms{init}
	{}

	void AddSymbol(SymbolPtr sym)
	{
		m_syms.push_back(sym);
	}

	std::size_t NumSymbols() const { return m_syms.size(); }

	const SymbolPtr operator[](const std::size_t i) const { return m_syms[i]; }

	bool operator==(const Word& other) const
	{
		if(this->NumSymbols() != other.NumSymbols())
			return false;

		bool bMatch = true;
		for(std::size_t i=0; i<NumSymbols(); ++i)
		{
			if(*m_syms[i] != *other.m_syms[i])
			{
				bMatch = false;
				break;
			}
		}

		return bMatch;
	}


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
	void AddRule(const Word& rule)
	{
		m_rules.push_back(rule);
	}


	/**
	 * number of rules
	 */
	std::size_t NumRules() const { return m_rules.size(); }


	/**
	 * get a production rule
	 */
	const Word& GetRule(std::size_t i) const
	{
		return m_rules[i];
	}


	/**
	 * does this non-terminal have a rule which produces epsilon?
	 */
	bool HasEpsRule() const
	{
		for(const auto& rule : m_rules)
		{
			if(rule.NumSymbols()==1 && rule[0]->IsEps())
				return true;
		}
		return false;
	}


private:
	// production rules
	std::vector<Word> m_rules;
};


using TerminalPtr = std::shared_ptr<Terminal>;
using NonTerminalPtr = std::shared_ptr<NonTerminal>;


extern TerminalPtr g_eps;
extern TerminalPtr g_end;



/**
 * calculates the first set
 */
void calc_first(const NonTerminalPtr nonterm,
	std::map<std::string, std::set<SymbolPtr>>& _first,
	std::map<std::string, std::vector<std::set<SymbolPtr>>>* _first_per_rule=nullptr);


/**
 * calculates the follow set
 */
void calc_follow(const std::vector<NonTerminalPtr>& allnonterms,
	const NonTerminalPtr& start, const NonTerminalPtr nonterm,
	std::map<std::string, std::set<SymbolPtr>>& _first,
	std::map<std::string, std::set<SymbolPtr>>& _follow);


#endif
