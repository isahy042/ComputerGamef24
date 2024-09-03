#include "PPU466.hpp"
#include "Mode.hpp"
#include "read_write_chunk.hpp"
#include "data_path.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <set>
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
		if (!(tile_output.size() % tile_size == 0)) {
			cerr << "tile file error" << endl;
		}

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

		if (!(palette_output.size() % 16 == 0 && palette_output.size() / 16 <= 8)) {
			cerr << "palette file error" << endl;
		}

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
		// placing tiles 9 - 17, sprite 4-10
		uint8_t x = int8_t(player_at.x);
		uint8_t y = int8_t(player_at.y);

		for (uint8_t i = 0; i < 3; i++) {

			uint8_t index = 15 - (i * 3);
			ppu.sprites[4 + (i * 2)].x = x;
			ppu.sprites[4 + (i * 2)].y = y;
			ppu.sprites[4 + (i * 2)].index = index;
			ppu.sprites[4 + (i * 2)].attributes = ((uint8_t)ppu.tile_palette_map[index]);

			ppu.sprites[4 + (i * 2) + 1].x = x + 8;
			ppu.sprites[4 + (i * 2) + 1].y = y;
			ppu.sprites[4 + (i * 2) + 1].index = index+1;
			ppu.sprites[4 + (i * 2) + 1].attributes = ((uint8_t)ppu.tile_palette_map[index+1]);

			y += 8;
		}

		ppu.sprites[10].x = int8_t(player_at.x) + 2;
		ppu.sprites[10].y = int8_t(player_at.y) + 8;
		ppu.sprites[10].index = 11;
		ppu.sprites[10].attributes = ((uint8_t)ppu.tile_palette_map[11]);
		
	}

	// spawn cups
	vector<int> has_cup;
	vector<int> cup_storage;

	uint8_t cup_y = 60;

	void spawn_cup(float elapsed) {

		uint8_t spawn_at = 0;
		int spawn_type = -1;

		srand((unsigned int)(elapsed * (2 << 25)));

		int random_location = rand() % has_cup.size();
		int random_type = rand() % 13;

		// 0-7 small, 8-11 medium, 12 potion
		if (random_type == 12) random_type = 2;
		else if (random_type <= 7) random_type = 1;
		else random_type = 0;

		for (int i = 0; i < has_cup.size(); i++) {
			random_location = random_location % has_cup.size();
			if (has_cup[random_location] < 0) {
				spawn_at = (uint8_t)random_location;
				// decide spawn type
				for (int type = 0; type < 3; type++) {
					random_type = random_type % 3;
					if (cup_storage[random_type] >= 0) {
						spawn_type = (uint8_t)random_type;
						break;
					}
					random_type++;
				}
				break;
			}
			random_location++;
		}

		// sprite index
		if (spawn_type < 0) return;
		int index = cup_storage[spawn_type];

		switch (spawn_type) {
		case 0:
			ppu.sprites[20 + (2 * index)].x = 50 + (spawn_at * 8);
			ppu.sprites[20 + (2 * index)].y = cup_y;
			ppu.sprites[20 + 1 + (2 * index)].x = 50 + (spawn_at * 8);
			ppu.sprites[20 + 1 + (2 * index)].y = cup_y - 8;
			cup_storage[spawn_type]--;
			has_cup[spawn_at] = 20 + (2 * index);

			break;

		case 1:
			ppu.sprites[40 + index].x = 50 + (spawn_at * 8);
			ppu.sprites[40 + index].y = cup_y - 8;
			cup_storage[spawn_type]--;
			has_cup[spawn_at] = 40 + index;

			break;

		case 2:
			ppu.sprites[55 + (2 * index)].x = 50 + (spawn_at * 8);
			ppu.sprites[55 + (2 * index)].y = cup_y;
			ppu.sprites[55 + 1 + (2 * index)].x = 50 + (spawn_at * 8);
			ppu.sprites[55 + 1 + (2 * index)].y = cup_y - 8;
			cup_storage[spawn_type]--;
			has_cup[spawn_at] = 55 + (2 * index);

			break;
		}



	}

	set<int> dropping_cups;

	void try_push_cup() {

		// get left most index
		int index = ((int)player_at.x - 50) / 8;
		int tiles_covered = (((int)player_at.x - 50) % 8 == 0) ? 2 : 3;

		for (int i = index; i < index + tiles_covered; i++) {
			if (has_cup[i] > 0) {
				if (has_cup[i] < 40) score += 2;
				else if (has_cup[i] >= 55) player_speed += 10.f;
				else score += 1;
				dropping_cups.insert(has_cup[i]);
				has_cup[i] = -1;
			}
		}
	}

	// decrease y of all currently dropping cups
	void drop_cups(float elapsed) {
		for (auto cup : dropping_cups) {
			if (cup < 40 || cup >= 55) {
				ppu.sprites[cup].y -= 1;
				ppu.sprites[cup + 1].y -= 1;
				if (ppu.sprites[cup + 1].y <= 4) {
					ppu.sprites[cup].y = 250;
					ppu.sprites[cup + 1].y = 250;
					dropping_cups.erase(cup);
				}
			}
			else {
				ppu.sprites[cup].y -= 1;
				if (ppu.sprites[cup].y <= 4) {
					ppu.sprites[cup].y = 250;
					dropping_cups.erase(cup);
				}
			}
		}
	}

	int score = 0;
	float time = 30.f;
	float player_speed = 20.f;

};
