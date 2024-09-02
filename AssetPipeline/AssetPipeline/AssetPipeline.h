// AssetPipeline.h

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

const int MAX_PALETTES = 8; 
const int PALETTE_SIZE = 4;

bool cmp_color(vector<unsigned char> &c1, vector<unsigned char>& c2) {
	return (c1[0] == c2[0] &&
		c1[1] == c2[1] &&
		c1[2] == c2[2] &&
		c1[3] == c2[3]);
}

// array of 8 palettes, 4 colors in each, all initialized to {0,0,0,0}
vector<vector<vector<unsigned char>>> palettes(MAX_PALETTES, vector<vector<unsigned char>>(PALETTE_SIZE, vector<unsigned char>(4, 0)));
vector<int>palette_color_index(8, 1); // in true nintendo fashion, setting the first one to transparent!
int next_empty_palette = 0;


/*
	Given a palette and a set of colors, determine whether its possible to union them without exeeding the palette size
*/
bool palette_overlap(int p, set<vector<unsigned char>>& colors) {
	int spots_needed = 0;
	vector<vector<unsigned char>> palette = palettes[p];
	vector<vector<unsigned char>> new_colors;

	for (auto c : colors) {
		// if a color is not already in the palette, we would need a spot for it
		if (find(palette.begin(), palette.end(), c) == palette.end()) {
			spots_needed++;
			new_colors.push_back(c);
		}
	}
	

	// if there are enough spots, add the colors into the palette
	if (spots_needed <= PALETTE_SIZE - palette_color_index[p]) {
		for (auto c : new_colors) {
			assert(palette_color_index[p] < PALETTE_SIZE);
			palettes[p][palette_color_index[p]] = c;
			palette_color_index[p]++;
		}
		return true;
	}

	return false;
}

/* 
Process an 8x8 tile by matching it to a palette (or creating a new one if necessary)
*/
void process_8x8_tile(int i, int j, vector<vector<vector<unsigned char>>> &img, vector<unsigned char> &output) {

	// find corresponding palette by scanning the entire tile
	set<vector<unsigned char>> colors;

	for (int ii = i; ii < i + 8; ii++) {
		for (int jj = j; jj < j + 8; jj++) {
			colors.insert(img[ii][jj]);
			if (colors.size() == PALETTE_SIZE) break;
		}
		if (colors.size() == PALETTE_SIZE) break;
	}

	// compare exisitng palettes by seeing if colors is a subset of any of them
	int current_palette_index = -1;
	for (int p = 0; p < next_empty_palette; p++) {
		if (palette_overlap(p, colors)) {
			current_palette_index = p;
			break; 
		} 
	}
	
	// make new palette when necessary
	if (current_palette_index == -1) {

		if (next_empty_palette >= MAX_PALETTES) {
			next_empty_palette = MAX_PALETTES - 1;
			cout << "WARNING: Overwriting palettes while parsing, resetting last palette \n";
			palette_color_index[next_empty_palette] = 1;
		}

		current_palette_index = next_empty_palette;
		next_empty_palette++;
	

		for (auto c : colors) {
			assert(palette_color_index[current_palette_index] < PALETTE_SIZE);
			if (c[3] == 0) continue; // we don't have to add transparent
			palettes[current_palette_index][palette_color_index[current_palette_index]] = c;
			palette_color_index[current_palette_index]++;
		}
	}

	cout << "palette " << current_palette_index << " \n";

	vector<vector<unsigned char>> currentPalette = palettes[current_palette_index];

	//cout << "Palette Index " << current_palette_index << ", with colors: \n" 
	//	<<int( palettes[current_palette_index][0][0] )<< " " << int(palettes[current_palette_index][0][1]) << " "
	//	<< int(palettes[current_palette_index][0][2] )<< " " << int(palettes[current_palette_index][0][3] )<< " \n"
	//	<< int(palettes[current_palette_index][1][0] )<< " " << int(palettes[current_palette_index][1][1]) << " "
	//	<< int(palettes[current_palette_index][1][2] )<< " " << int(palettes[current_palette_index][1][3] )<< " \n"
	//	<< int(palettes[current_palette_index][2][0] )<< " " << int(palettes[current_palette_index][2][1] )<< " "
	//	<< int(palettes[current_palette_index][2][2]) << " " << int(palettes[current_palette_index][2][3] )<< " \n"
	//	<< int(palettes[current_palette_index][3][0] )<< " " << int(palettes[current_palette_index][3][1] )<< " "
	//	<< int(palettes[current_palette_index][3][2]) << " " << int(palettes[current_palette_index][3][3] )<< " \n";

	// push correct output onto output vector
	// first push the palette index
	output.push_back(current_palette_index);

	for (int ii = i; ii < i + 8; ii++) {
		for (int jj = j; jj < j + 8; jj++) {

			/*cout << "Pixel" <<ii<<" "<<jj<<" with colors : \n"
				<< int(img[ii][jj][0]) << " " << int(img[ii][jj][1]) << " "
				<< int(img[ii][jj][2] )<< " " << int(img[ii][jj][3] )<< " \n";*/

			if (cmp_color(img[ii][jj], currentPalette[0])) {
				output.push_back(0); output.push_back(0);
			}
			else if (cmp_color(img[ii][jj],currentPalette[1])) {
				output.push_back(0); output.push_back(1);
			}
			else if (cmp_color(img[ii][jj],currentPalette[2])) {
				output.push_back(1); output.push_back(0);
			}
			else if (cmp_color(img[ii][jj],currentPalette[3])) {
				output.push_back(1); output.push_back(1);
			}
			else {
				assert(false); // this should not be reached
			}
		}
	}

}



/* 
Read a png containing 8x8 tiles.
reusing some code from my 672 vulkan renderer spring 2024 
*/
vector<vector<vector<unsigned char>>> read_8x8_tile_png(string inFileName, int &inWidth, int &inHeight) {

	int texChannels;
	string in = "asset/" + inFileName;

	const char* inc = in.c_str();
	// load in image
	stbi_uc* pixels = stbi_load(inc, &inWidth, &inHeight, &texChannels, STBI_rgb_alpha);

	if (inHeight % 8 != 0 || inWidth % 8 != 0) cerr << "Input image not suitable for 8 x 8 tiles.";

	// store in 3D vector - height x width x 4
	vector<vector<vector<unsigned char>>> inImg(inHeight, vector<vector<unsigned char>>(inWidth, {0,0,0,0}));

	int p = 0;
	for (int h = 0; h < inHeight; h++) {
		for (int w = 0; w < inWidth; w++) {
			inImg[h][w] = { pixels[(p * 4)], pixels[(p * 4) + 1], pixels[(p * 4) + 2], pixels[(p * 4) + 3] };
			p++;
		}
	}

	return inImg;

}



/*
	NOTE: the write_chunk() helper function is from the starter code of game 1 from read_write_chunk.hpp
*/
//helper function that reads an array of structures preceded by a simple header:
//Expected format:
// |ma|gi|c.|..| <-- four byte "magic number"
// |sz|sz|sz|sz| <-- four byte (native endian) size
// |TT...TT| * (sz/sizeof(TT)) <-- enough T structures to make up sz bytes
template< typename T >
void write_chunk(std::string const& magic, std::vector< T > const& from, std::ofstream* to_) {
	assert(magic.size() == 4);
	assert(to_);
	auto& to = *to_;

	struct ChunkHeader {
		char magic[4] = { '\0', '\0', '\0', '\0' };
		uint32_t size = 0;
	};
	static_assert(sizeof(ChunkHeader) == 8, "header is packed");
	ChunkHeader header;
	header.magic[0] = magic[0];
	header.magic[1] = magic[1];
	header.magic[2] = magic[2];
	header.magic[3] = magic[3];
	header.size = uint32_t(from.size() * sizeof(T));

	to.write(reinterpret_cast<const char*>(&header), sizeof(header));
	
	cout << "writing, size is " << from.size() * sizeof(T) << "\n";
	
	to.write(reinterpret_cast<const char*>(from.data()), from.size() * sizeof(T));
}

