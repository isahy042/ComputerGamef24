#include "PPU466.hpp"
#include "Mode.hpp"
#include "read_write_chunk.hpp"
#include "data_path.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <deque>

using namespace std;

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, space;

	//some weird background animation:
	float background_fade = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	//----- drawing handled by PPU466 -----

	PPU466 ppu;

	//----- Asset Loading Helper Functions -----
	void set_tiles(string file, string magic, int start_tile) {

		ifstream infile;
		infile.open(data_path("asset/" + file));
		vector<unsigned char> tile_output;
		read_chunk(infile, magic, &tile_output);
		infile.close();

		int tile_size = ((8 * 8 * 2) + 1);
		assert(tile_output.size() % tile_size == 0);

		size_t total_tiles = tile_output.size() / tile_size;

		for (size_t i = 0; i < total_tiles; i++) {
			PPU466::Tile tile;

			// the current index in the tile_output vectoor
			size_t vector_index = i * tile_size;

			// set palette for initial color
			ppu.tile_palette_map[start_tile + i] = int(tile_output[vector_index]);
			vector_index++;

			// fill in each row of the bit table
			for (int j = 0; j < 8; j++) {
				string bit0 ="";
				string bit1 = "";
				for (int k = 0; k < 8; k++) {
					bit1 += (char)(int(tile_output[vector_index]) + '0');
					vector_index++;
					bit0 += (char)(int(tile_output[vector_index]) + '0');
					vector_index++;
				}

				// we have a binary string, cast to uint8 and assign it
				tile.bit0[j] = (uint8_t)stoi(bit0.c_str(), nullptr, 2);
				tile.bit1[j] = (uint8_t)stoi(bit1.c_str(), nullptr, 2);
			}

			ppu.tile_table[start_tile + i] = tile;
		}
	}

	// given vector data, fill in all palettes
	void set_palette() {

		ifstream infile;
		infile.open(data_path("asset/pale"));
		vector<unsigned char> palette_output;
		read_chunk(infile, "pale", &palette_output);
		infile.close();

		assert(palette_output.size() % 16 == 0 && palette_output.size() / 16 <= 8);

		// fill in used palettes
		size_t index = 0;
		for (size_t p = 0; p < palette_output.size() / 16; p++) {
			for (int i = 0; i < 4; i++) {
				ppu.palette_table[p][i] = glm::u8vec4(
					int(palette_output[index])
					, int(palette_output[index+1])
					, int(palette_output[index+2])
					, int(palette_output[index+3]));
				index += 4;
			}
		}

		// unused palettes
		for (size_t p = palette_output.size() / 16; p < ppu.palette_table.size(); p++) {
			ppu.palette_table[p] = {
				glm::u8vec4(0x00, 0x00, 0x00, 0x00),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00),
			};
		}

	}

	//----- Runtime Helper Functions -----
	// set position of cat
	void set_cat() {
	}

	void spawn_cup() {
	}

	void try_push_cup() {
	}

	int score = 0;
	float time = 60.f;
};
