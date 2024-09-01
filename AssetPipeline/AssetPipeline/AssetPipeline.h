// AssetPipeline.h

#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum assetType {
	Tile8x8,
	Palette2x2,
	Generic
};

/* reusing some code from my 672 vulkan renderer spring 2024 */
void read_png(string inFileName) {

	int inWidth, inHeight, texChannels;
	string in = "asset/" + inFileName;

	const char* inc = in.c_str();
	// load in image
	stbi_uc* pixels = stbi_load(inc, &inWidth, &inHeight, &texChannels, STBI_rgb_alpha);

	if (inHeight & 8 != 0 || inWidth % 8 != 0) cerr << "Input image not suitable for 8 x 8 tiles.";

	// store in 3D vector - height x width x 4
	vector<vector<vector<unsigned char>>> inImg(inHeight, vector<vector<unsigned char>>(inWidth, vector<unsigned char>(4, 0)));

	int p = 0;
	for (int h = 0; h < inHeight; h++) {
		for (int w = 0; w < inWidth; w++) {
			inImg[h][w] = {pixels[(p * 4)], pixels[(p * 4) + 1], pixels[(p * 4) + 2], pixels[(p * 4) + 3]};
			p++;
		}
	}

}

/*
	NOTE: the write_chunk() helper function is from the starter code of game 1 from read_write_chunk.hpp
*/

//helper function that reads an array of structures preceded by a simple header:
//Expected format:
// |ma|gi|c.|..| <-- four byte "magic number"
// |sz|sz|sz|sz| <-- four byte (native endian) size
// |TT...TT| * (sz/sizeof(TT)) <-- enough T structures to make up sz bytes

//helper function to write a chunk of data:
template< typename T >
void write_chunk(std::string const& magic, std::vector< T > const& from, std::ostream* to_) {
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
	to.write(reinterpret_cast<const char*>(from.data()), from.size() * sizeof(T));
}

