/**
 * run vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 19-jun-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "vm/vm.h"

#include <vector>
#include <iostream>
#include <fstream>

#if __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#elif __has_include(<boost/filesystem.hpp>)
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif



static bool run_vm(const char* _prog = nullptr)
{
	fs::path prog = _prog;
	std::size_t filesize = fs::file_size(prog);

	std::ifstream ifstr(prog, std::ios_base::binary);
	if(!ifstr)
		return false;

	std::vector<VM::t_byte> bytes;
	bytes.resize(filesize);

	ifstr.read(reinterpret_cast<char*>(bytes.data()), filesize);
	if(ifstr.fail())
		return false;

	try
	{
		VM vm(4096);
		//vm.SetDebug(true);
		vm.SetMem(0, bytes.data(), filesize);
		vm.Run();
		VM::t_data dat = vm.TopData();
		std::cout << "Top of stack: ";
		std::visit([](auto&& val) -> void
		{
			using t_val = std::decay_t<decltype(val)>;

			std::cout << val
				<< " ["
				<< vm_type_name<t_val>
				<< "]";
		}, dat);
		std::cout << std::endl;
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return false;
	}

	return true;
}



int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	std::ios_base::sync_with_stdio(false);

	if(argc <= 1)
	{
		std::cerr << "Please give a compiled program." << std::endl;
		return -1;
	}

	if(!run_vm(argv[1]))
	{
		std::cerr << "Could not run \"" << argv[1] << "\"." << std::endl;
		return -1;
	}

	return 0;
}
