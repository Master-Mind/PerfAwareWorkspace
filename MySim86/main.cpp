#include <sim86_shared.h>
#include <print>
#include <argparse/argparse.hpp>
#include <fstream>
#include <filesystem>

std::vector<u8> loadInput(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();

	file.seekg(0, std::ios::beg);

	std::vector<u8> ret(size);
	if (!file.read(reinterpret_cast<char*>(ret.data()), size))
	{
		throw std::exception("Failed to read file!");
	}

	return ret;
}

int main(int argc, char *argv[])
{
	//std::print("Hello {0}", Sim86_GetVersion());
	argparse::ArgumentParser program("My Sim 86");

	program.add_argument("-i", "--input");

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;

		return 0;
	}

	auto filedata = loadInput(program.get("-i"));

	instruction instr;
	Sim86_Decode8086Instruction(filedata.size(), filedata.data(), &instr);

	return 0;
}