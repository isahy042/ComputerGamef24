// AssetPipeline.cpp : Defines the entry point for the application.
//

#include "AssetPipeline.h"

using namespace std;

int main(int argc, char *argv[])
{
	// parsing arguments in the main function
	// * reusing some code from my 672 vulkan renderer spring 2024
	bool validArgs = false;

	vector<string> filePaths;
	vector<string> magics;

	for (int i = 1; i < argc; i++) {
		string argument = argv[i];
		if (argument == "-f") { // file
			while (i + 1 < argc && argv[i + 1][0] != '-') {
				try {
					filePaths.push_back(string(argv[i + 1]));
				}
				catch (const std::exception& e) {
					std::cerr << e.what() << std::endl;
					exit(1);
				}
				i++;
			}
			validArgs = true;
			
		}
		else if (argument == "-m") {// magic values
			while (magics.size() < filePaths.size()) {
				if (!validArgs) {
					cout << "No file or incorrect input order. Use -h to see the help message. \n";
					exit(1);
				}
				// use command line input for magic value
				if (i + 1 < argc && argv[i + 1][0] != '-') {
					try {
						// ensure magic is 4 chars long
						magics.push_back((string(argv[i + 1]) + string("    ")).substr(0, 4));
					}
					catch (const std::exception& e) {
						std::cerr << "invalid argument." << std::endl;
						exit(1);
					}
				}
				else {
					// "If magic is not provided, first 4 chars of the file name is used"
					// use corresponding file name for magic value
					magics.push_back((filePaths[magics.size()] + string("    ")).substr(0, 4));
				}
				i++;
			}
			
		}
		else if (argument == "-h") {// culling
			cout << "AssetPipeline -f [file1] [file2].. -m [magic1] [magic2].. \n" << 
				"If magic is not provided, first 4 chars of the file name is used. Use of -m still required. ";
			exit(1);
		}
	}

	// check input correctness
	if (!validArgs || magics.size() != filePaths.size()) {
		cout << "Invalid argument used. Use -h to see the help message.\n";
		exit(1);
	}

	// process the input
	cout << "Your input: \n";
	for (int i = 0; i < filePaths.size(); i++) {
		cout << "File:" << filePaths[i] + " Magic:[" << magics[i] << "]\n";
	}

	exit(0);
	// read in the target file

    
}
