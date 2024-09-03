#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

PlayMode::PlayMode() {

	/* Asset Loading */
	// read in the tiles
	// specific to this game: cat cups moon nums txtr
	// use tiles 0 - 8 as moon
	set_tiles(string("moon"), string("moon"), 0);

	// use tiles 9 - 17 as cat
	set_tiles(string("cat"), string("cat "), 9);

	// use tiles 18 - 23 as cups
	set_tiles(string("cups"), string("cups"), 18);

	// use tiles 24 - 29 as texture
	set_tiles(string("txtr"), string("txtr"), 24);

	// use tiles 30-39 as nums
	set_tiles(string("nums"), string("nums"), 30);

	// read in the palettes
	// palette contains 4 * 4 * P bytes of data
	set_palette();

	/* Set Background */
	// 64x60 tiles
	// roughly divide into wall and table 
	uint16_t background;
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		
		// hardcode in relevant background indices
		uint16_t tile_index = 25;// normal wall
		if (y == 0) tile_index = 29; // bottom row wall
		else if (y == 1) tile_index = 27; // second row wall
		else if (y >= 6 && y <= 11) tile_index = 28 - (((y - 6) % 3) * 2); // table, 3 tiles in a row
		
		background = (((uint16_t)ppu.tile_palette_map[tile_index]) << 8) | tile_index;

			
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * y] = background;
		}
	}

	// add night sky
	for (uint32_t y = 17; y <= 26; y++) {

		// hardcode in relevant background indices
		uint16_t tile_index = 3;// normal night

		for (uint32_t x = 14; x <= 30; x++) {
			if ((x == 22) && (y == 24)) tile_index = 0;
			else if ((x == 23) && (y == 24)) tile_index = 1;
			else if ((x == 24) && (y == 24)) tile_index = 2;
			else if ((x == 23) && (y == 23)) tile_index = 4;
			else if ((x == 24) && (y == 23)) tile_index = 5;
			else if ((x == 22) && (y == 22)) tile_index = 6;
			else if ((x == 23) && (y == 22)) tile_index = 7;
			else if ((x == 24) && (y == 22)) tile_index = 8;
			else tile_index = 3;

			background = (((uint16_t)ppu.tile_palette_map[tile_index]) << 8) | tile_index;
			ppu.background[x + PPU466::BackgroundWidth * y] = background;
		}
	}

	/* Set cups using sprite 20 - 60 */
	// starting at x = 50, about 18 slots for cups
	has_cup = vector<int>(18, -1);

	for (int i = 20; i < 40; i += 2) {
		ppu.sprites[i].index = 18;
		ppu.sprites[i].attributes = ((uint8_t)ppu.tile_palette_map[18]);
		ppu.sprites[i+1].index = 21;
		ppu.sprites[i + 1].attributes = ((uint8_t)ppu.tile_palette_map[21]);
	}
	cup_storage.push_back(10-1);

	for (int i = 40; i < 55; i ++) {
		ppu.sprites[i].index = 19;
		ppu.sprites[i].attributes = ((uint8_t)ppu.tile_palette_map[19]);
	}
	cup_storage.push_back(15-1);

	for (int i = 55; i < 61; i += 2) {
		ppu.sprites[i].index = 20;
		ppu.sprites[i].attributes = ((uint8_t)ppu.tile_palette_map[20]);
		ppu.sprites[i + 1].index = 23;
		ppu.sprites[i + 1].attributes = ((uint8_t)ppu.tile_palette_map[23]);
	}
	cup_storage.push_back(3-1);



	player_at.y = 55; // cat has fixed horizontal path
	player_at.x = 120;
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		} 
	}

	return false;
}

void PlayMode::update(float elapsed) {

	int second1 = (int)floor(time);
	time -= elapsed;

	int second2 = (int)floor(time);
	if ((second1 != second2) && (second1 % 3 == 0)) spawn_cup(elapsed);

	if (left.pressed) player_at.x = max(player_at.x - (player_speed * elapsed), 50.f);
	if (right.pressed) player_at.x = min(player_at.x + (player_speed * elapsed), 190.f);
	if (space.pressed) try_push_cup();

	// TODO: lower the dropping cups, recycle if hit the floor

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	space.downs = 0;

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---
	// 
	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = 0;

	//uint8_t bg_priority = 1 << 7;
	uint8_t num_attribute = ((uint8_t)ppu.tile_palette_map[30]);

	// update timer - left of screen 0-1
	string time_str = to_string(time);
	ppu.sprites[0].x = 12;
	ppu.sprites[0].y = 225;
	ppu.sprites[0].index = 30 + (((int)ceil(time) / 10 ) % 10);
	ppu.sprites[0].attributes = num_attribute;
	ppu.sprites[1].x = 19;
	ppu.sprites[1].y = 225;
	ppu.sprites[1].index = 30 + ((int)ceil(time) % 10);
	ppu.sprites[1].attributes = num_attribute;

	// score 2-3
	ppu.sprites[2].x = 220;
	ppu.sprites[2].y = 225;
	ppu.sprites[2].index = 30 + ((score / 10) % 10);
	ppu.sprites[2].attributes = num_attribute;
	ppu.sprites[3].x = 228;
	ppu.sprites[3].y = 225;
	ppu.sprites[3].index = 30 + (score % 10);
	ppu.sprites[3].attributes = num_attribute;

	// player sprite 4-10
	set_cat();
	//ppu.sprites[1].x = int8_t(player_at.x);
	//ppu.sprites[0].y = int8_t(player_at.y);
	//ppu.sprites[0].index = ;
	//ppu.sprites[0].attributes = 7;

	////some other misc sprites:
	//for (uint32_t i = 1; i < 63; ++i) {
	//	float amt = (i + 2.0f * background_fade) / 62.0f;
	//	ppu.sprites[i].x = int8_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * PPU466::ScreenWidth);
	//	ppu.sprites[i].y = int8_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * PPU466::ScreenWidth);
	//	ppu.sprites[i].index = 32;
	//	ppu.sprites[i].attributes = 6;
	//	if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	//}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
