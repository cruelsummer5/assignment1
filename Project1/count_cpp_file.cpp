#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

using namespace std;
namespace fs = std::experimental::filesystem;

struct DirCxxBreif
{
	DirCxxBreif(): 
		fileCount(0),
		lineCount(0),
		emptyLineCount(0),
		headerFileCount(0),
		sourceFileCount(0)
		{}
	int fileCount;
	int lineCount;
	int emptyLineCount;
	int headerFileCount;
	int sourceFileCount;
};

pair<int,int> count_lines(const string& filename)
{
	ifstream file(filename);
	int count = 0;
	int emptyCount = 0;
	string line;
	while (getline(file, line)) {
		++count;
		if (line.empty()) ++emptyCount;
	}

	return {count, emptyCount};
}

bool is_cpp_header_file(const fs::directory_entry& entry) 
{
	return entry.path().extension() == ".h" || entry.path().extension() == ".hpp" || entry.path().extension() == ".hxx";
}

bool is_cpp_source_file(const fs::directory_entry& entry)
{
	return entry.path().extension() == ".cc" || entry.path().extension() == ".cpp" || entry.path().extension() == ".cxx" || entry.path().extension() == ".c++";
}

void count_cpp_files(const fs::path& dir, DirCxxBreif& dcb) 
{
	for (const auto& entry : fs::directory_iterator(dir)) 
	{
		bool isCppFile = false;
		if (fs::is_directory(entry)) count_cpp_files(entry.path(), dcb);
		else if (is_cpp_header_file(entry)) 
		{
			++dcb.headerFileCount;
			isCppFile = true;
		}
		else if (is_cpp_source_file(entry)) 
		{
			++dcb.sourceFileCount;
			isCppFile = true;
		}

		if (isCppFile) 
		{
			dcb.fileCount++;

			auto pr = count_lines(entry.path().string());
			dcb.lineCount += pr.first;
			dcb.emptyLineCount += pr.second;

			cout << entry.path() << ": " << pr.first << " lines, " << pr.second << " empty lines\n";
		}
	}
}

int main(int argc, char* argv[]) 
{
	if (argc < 2) 
	{
		cout << "Usage: " << argv[0] << " <directory>" << endl;
		return 1;
	}

	fs::path dir(argv[1]);

	if (!fs::is_directory(dir)) 
	{
		cout << "Not a directory: " << dir << endl;
		return 1;
	}

	DirCxxBreif dcb;

	auto start_time = chrono::high_resolution_clock::now();
	count_cpp_files(dir, dcb);
	auto end_time = chrono::high_resolution_clock::now();

	auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

	cout << "\n\nelapsed time: " << duration.count() << "ms" << endl;

	cout << dcb.fileCount << " C++ files, " << dcb.lineCount << " lines, "
		<< dcb.emptyLineCount << " empty lines, " << dcb.headerFileCount
		<< " header files, " << dcb.sourceFileCount << " source files" << endl;

	system("pause");
	return 0;
}