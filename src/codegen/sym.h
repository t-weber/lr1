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
	t_vm_addr addr{0};             // relative address of the variable
	VMType loc{VMType::ADDR_BP};   // register with the base address

	VMType ty{VMType::UNKNOWN};    // data type
	bool is_func{false};           // function or variable

	t_vm_addr arg_size{0};         // size of arguments
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
		t_vm_addr addr, VMType loc = VMType::ADDR_BP,
		VMType ty = VMType::UNKNOWN, bool is_func = false,
		t_vm_addr arg_size = 0)
	{
		SymInfo info
		{
			.addr = addr,
			.loc = loc,
			.ty = ty,
			.is_func = is_func,
			.arg_size = arg_size,
		};

		return &m_syms.insert_or_assign(name, info).first->second;
	}


private:
	std::unordered_map<std::string, SymInfo> m_syms{};
};


#endif
