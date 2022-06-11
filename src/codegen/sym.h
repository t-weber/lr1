/**
 * symbol table
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __LR1_SYMTAB_H__
#define __LR1_SYMTAB_H__


#include <unordered_map>

#include "../vm/opcodes.h"


/**
 * information about a variable in the symbol table
 */
struct SymInfo
{
	t_vm_addr addr{0};               // relative address of the variable
	VMRegister loc{VMRegister::BP};  // register with the base address
};


/**
 * symbol table mapping an identified to an address
 */
class SymTab
{
public:
	SymTab() {}
	~SymTab() {}


	const SymInfo* GetSymbol(const std::string& name) const
	{
		if(auto iter = m_syms.find(name); iter != m_syms.end())
			return &iter->second;

		return nullptr;
	}


	const SymInfo* AddSymbol(const std::string& name,
		t_vm_addr addr, VMRegister loc = VMRegister::BP)
	{
		SymInfo info
		{
			.addr = addr,
			.loc = loc,
		};

		return &m_syms.insert_or_assign(name, info).first->second;
	}


private:
	std::unordered_map<std::string, SymInfo> m_syms{};
};


#endif
