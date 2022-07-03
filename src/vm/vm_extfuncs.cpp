/**
 * external functions
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 3-july-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "vm.h"


/**
 * call external function
 */
VM::t_data VM::CallExternal(const t_str& func_name)
{
	VM::t_data retval;

	if(m_debug)
	{
		std::cout << "Calling external function \"" << func_name << "\""
			//<< " with " << num_args << " arguments."
			<< std::endl;
	}

	if(func_name == "sqrt")
	{
		OpCast<t_real>();
		t_real arg = std::get<t_real>(PopData());
		retval = std::sqrt(arg);
	}
	else if(func_name == "pow")
	{
		OpCast<t_real>();
		t_real arg1 = std::get<t_real>(PopData());
		OpCast<t_real>();
		t_real arg2 = std::get<t_real>(PopData());
		retval = std::pow(arg1, arg2);
	}
	else if(func_name == "sin")
	{
		OpCast<t_real>();
		t_real arg = std::get<t_real>(PopData());
		retval = std::sin(arg);
	}
	else if(func_name == "cos")
	{
		OpCast<t_real>();
		t_real arg = std::get<t_real>(PopData());
		retval = std::cos(arg);
	}
	else if(func_name == "tan")
	{
		OpCast<t_real>();
		t_real arg = std::get<t_real>(PopData());
		retval = std::tan(arg);
	}
	else if(func_name == "print")
	{
		OpCast<t_str>();
		const t_str/*&*/ arg = std::get<t_str>(PopData());
		std::cout << arg;
		std::cout.flush();
	}
	else if(func_name == "input_real")
	{
		t_real val{};
		std::cin >> val;
		retval = val;
	}
	else if(func_name == "input_int")
	{
		t_int val{};
		std::cin >> val;
		retval = val;
	}

	return retval;
}
