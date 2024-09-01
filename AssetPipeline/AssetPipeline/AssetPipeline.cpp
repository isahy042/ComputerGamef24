// AssetPipeline.cpp : Defines the entry point for the application.
//

#include "AssetPipeline.h"

using namespace std;

int main(int argc, char *argv[])
{
	// parsing arguments in the main function
	// * reusing some code from my 672 vulkan renderer spring 2024
	bool validArgs = false;

	vector<string> fileNames;
	vector<string> magics;

	for (int i = 1; i < argc; i++) {
		string argument = argv[i];
		if (argument == "-f") { // file
			while (i + 1 < argc && argv[i + 1][0] != '-') {
				try {
					fileNames.push_back(string(argv[i + 1]));
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
			while (magics.size() < fileNames.size()) {
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
						std::cerr << e.what() << std::endl;
						exit(1);
					}
				}
				else {
					// "If magic is not provided, first 4 chars of the file name is used"
					// use corresponding file name for magic value
					magics.push_back((fileNames[magics.size()] + string("    ")).substr(0, 4));
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
	if (!validArgs || magics.size() != fileNames.size()) {
		cout << "Invalid argument used. Use -h to see the help message.\n";
		exit(1);
	}

	// process the input
	cout << "Your input: \n";
	for (int i = 0; i < fileNames.size(); i++) {
		cout << "File:" << fileNames[i] + " Magic:[" << magics[i] << "]\n";
	}

	// read in the target file
	for (int i = 0; i < fileNames.size(); i++) {
		// declare output vector
		vector<unsigned char> output;

		int w, h;
		auto img = read_8x8_tile_png(fileNames[i], w, h);
		
		int tileIndex = 0;

		// process each tile
		for (int j = 0; j < h; j += 8) {
			for (int k = 0; k < w; k += 8) {
				cout << "Processing tile #" << tileIndex <<":\n";
				process_8x8_tile(j, k, img, output);
				tileIndex++;
			}
		}

		std::ofstream file;
		file.open("asset/"+ magics[i]);
		// save all tiles
		write_chunk(magics[i], output, &file);
		file.close();

	}
	
	// put all palettes into one array
	vector<unsigned char> output_palette;
	for (int i = 0; i < emptyPaletteIndex; i++) {
		vector<vector<unsigned char>> palette = palettes[i];
		for (int color = 0; color < PALETTE_SIZE; color++) {
			output_palette.push_back(palette[color][0]);
			output_palette.push_back(palette[color][1]);
			output_palette.push_back(palette[color][2]);
			output_palette.push_back(palette[color][3]);
		}
	}

	std::ofstream file;
	file.open("asset/pale");

	// save all palettes
	write_chunk("pale", output_palette, &file);
	file.close();

}
