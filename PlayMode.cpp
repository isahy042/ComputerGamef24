#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("room.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("room.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

/* All sound samples */
Load< Sound::Sample > knock_door_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});

Load< Sound::Sample > hit_door_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});

Load< Sound::Sample > use_key_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});

Load< Sound::Sample > knock_wall_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});

Load< Sound::Sample > knock_wall_hollow_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});

Load< Sound::Sample > knock_wall_safe_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});

Load< Sound::Sample > open_wall_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});

Load< Sound::Sample > collect_key_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("dusty-floor.opus"));
	});



PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name.substr(0,4) == "Wall") {
			walls[stoi(transform.name.substr(5, 3))] = &transform;
		}
		if (transform.name.substr(0, 3) == "Box") {
			boxes[stoi(transform.name.substr(4, 3))] = &transform;
		}
		else if (transform.name == "Door") door = &transform;
		else if (transform.name == "Safe") safe = &transform;
		else if (transform.name == "Key_Safe") key1 = &transform;
		else if (transform.name == "Key_Door") key2 = &transform;
		else if (transform.name == "item_box") item_holder = &transform;
		else if (transform.name == "Selector_Item") item_selector = &transform;
		else if (transform.name == "Selector_Wall") selector = &transform;
		else if (transform.name == "Selector_Door") door_selector = &transform;

	}

	if (door == nullptr) throw std::runtime_error("Mesh not found.");
	if (safe == nullptr) throw std::runtime_error("Mesh not found.");
	if (key1 == nullptr) throw std::runtime_error("Mesh not found.");
	if (key2 == nullptr) throw std::runtime_error("Mesh not found.");
	if (item_holder == nullptr) throw std::runtime_error("Mesh not found.");
	if (item_selector == nullptr) throw std::runtime_error("Mesh not found.");
	if (selector == nullptr) throw std::runtime_error("Mesh not found.");
	if (door_selector == nullptr) throw std::runtime_error("Mesh not found.");
	for (auto w : walls) if (w == nullptr) throw std::runtime_error("Wall not found.");
	for (auto b : boxes) if (b == nullptr) throw std::runtime_error("Box not found.");

	selector_base_pos = selector->position;
	door_selector_base_pos = door_selector->position;
	item_selector_base_pos = item_selector->position;

	// initialize variables
	item_list = { true, true, false, false };

	// move things offscreen
	door_selector->position = out_of_screen;
 
	// find a box for key 1 and a box for key 2
	int key1_box = 21;
	int key2_box = 10;

	// find two random box that will be empty
	int emp1 = 5;
	int emp2 = 11;

	walls_occupied = std::vector<int>(total_walls, 0);
	walls_occupied[emp1] = 3;
	walls_occupied[emp2] = 3;
	walls_occupied[key1_box] = 1;
	walls_occupied[key2_box] = 2;

	// move key and safe to appropriate boxes
	key1->position = boxes[key1_box]->position;
	key2->position = boxes[key2_box]->position;
	safe->position = boxes[key2_box]->position;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	////start music loop playing:
	//// (note: position will be over-ridden in update())
	//sound_source = Sound::play(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
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
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_1) {
			one.downs += 1;
			one.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_2) {
			two.downs += 1;
			two.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_3) {
			three.downs += 1;
			three.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_4) {
			four.downs += 1;
			four.pressed = true;
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
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_1) {
			one.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_2) {
			two.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_3) {
			three.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_4) {
			four.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	time += elapsed;

	if (one.pressed) toggle_item(1);
	else if (two.pressed) toggle_item(2);
	else if (three.pressed) toggle_item(3);
	else if (four.pressed) toggle_item(4);

	// slow down toggle time - once every 0.075 s
	if ((int)(time * 13) != increment) {
		increment = (int)(time * 13);
		toggle_wall(left.pressed, right.pressed, up.pressed, down.pressed);
		if (space.pressed) interact();
	}

	if (crow_bar == 0) {
		// TODO: Game Over
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	// reset timer when number too high
	if (time > 600.f) {
		time = 0;
		increment = 0;
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

/* Convert row col to wall index because they don't correspond to each other :( */
int PlayMode::get_wall_index() {

	return 0;
}

/* Use 1-4 to toggle item selection */
void PlayMode::toggle_item(int item) {

	item--; // make item 0 indexed

	if (!item_list[item] || item_index == item) return;

	item_index = item;
	item_selector->position = item_selector_base_pos + (glm::vec3(0.f, 0.f, -0.55f) * (float)item);
}

/* Use arrow to toggle wall selection */
void PlayMode::toggle_wall(bool isLeft, bool isRight, bool isUp, bool isDown) {

	if ((isUp == isDown) && (isLeft == isRight)) return;
	
	// if currently selecting door
	bool isDoor = (selected_row < 4) && (selected_col == 3 || selected_col == 4);

	if (isDoor) {
		if (isLeft && !isRight) selected_col = 2;
		else if (!isLeft && isRight) selected_col = 5;
		if (isUp && !isDown) selected_row = 4;
		
		if (isDown && (isLeft == isRight)) return;
	}
	else {
		if (isUp && !isDown) selected_row = (selected_row >= 4) ? 4 : selected_row + 1;
		else if (!isUp && isDown) selected_row = (selected_row <= 0) ? 0 : selected_row - 1;
		if (isLeft && !isRight) selected_col = (selected_col <= 0) ? 0 : selected_col - 1;
		else if (!isLeft && isRight) selected_col = (selected_col >= 7) ? 7 : selected_col + 1;

		// is the door selected?
		isDoor = (selected_row < 4) && (selected_col == 3 || selected_col == 4);
		if (isDoor) {
			door_selector->position = door_selector_base_pos;
			selector->position = out_of_screen;
			return;
		}
	}

	// select corresponding wall tile
	door_selector->position = out_of_screen;
	selector->position = selector_base_pos
		+ (glm::vec3(0.f, 0.f, 1.f) * (float)selected_row)
		+ (glm::vec3(1.f, 0.f, 0.f) * (float)selected_col);

	printf("%d %d \n", selected_row, selected_col);

	

}

/* Interact with something*/
void PlayMode::interact() {
	// 0 fist
	// 1 crow bar
	// 2 key 1
	// 3 key 2

	// walls_occupied[wall_index]
	// -1 = opened, solid
	// -2 = opened, key #1
	// -3 = opened, safe not opened
	// -4 = opened, hollow
	// -5 = opened, key #2


	bool isDoor = (selected_row < 4) && (selected_col == 3 || selected_col == 4);

	if (isDoor) {
		if (item_index == 0) {
			sound_source = Sound::play(*knock_door_sample);
			interaction_str = "Maybe someone will open the door for me.";
		}
		else if (item_index == 1) {
			crow_bar--;
			sound_source = Sound::play(*hit_door_sample);
			interaction_str = "This door is too sturdy. I should look for a key.";
		}
		else if (item_index == 2) {
			sound_source = Sound::play(*use_key_sample);
			interaction_str = "It won't open. This key looks too new for the door.";
		}
		else if (item_index == 3) {
			sound_source = Sound::play(*use_key_sample);
			interaction_str = "It worked!";
			// TODO: Game over.
		}
		else assert(false); // should not reach here
	}
	else {
		int wall_index = get_wall_index();
		if (item_index == 0) {

			if (walls_occupied[wall_index] == 0) {
				sound_source = Sound::play(*knock_wall_sample);
				interaction_str = "...";
			}

			else if (walls_occupied[wall_index] == 1) {
				sound_source = Sound::play(*knock_wall_hollow_sample);
				interaction_str = "...huh.";
			}

			else if (walls_occupied[wall_index] == 2) {
				sound_source = Sound::play(*knock_wall_safe_sample);
				interaction_str = "...huh.";
			}

			else if (walls_occupied[wall_index] == 3) {
				sound_source = Sound::play(*knock_wall_hollow_sample);
				interaction_str = "...huh.";
			}

			else if (walls_occupied[wall_index] == -2) {
				// collect key 1
				sound_source = Sound::play(*collect_key_sample);
				interaction_str = "Got a key.";
			}

			else if (walls_occupied[wall_index] == -5) {
				// collect key 2
				sound_source = Sound::play(*collect_key_sample);
				interaction_str = "Got a key.";
			}
			
		}
		else if (item_index == 1) {
			
			if (walls_occupied[wall_index] == 0) {
				crow_bar--;

				walls_occupied[wall_index] = -1;

				boxes[wall_index]->rotation *= glm::angleAxis(
					glm::radians(180.f),
					glm::vec3(0.0f, 0.0f, 1.0f));
				walls[wall_index]->position += out_of_screen;

				sound_source = Sound::play(*open_wall_sample);
				interaction_str = "The wall here is solid.";

			}
			else if (walls_occupied[wall_index] == 1) {
				crow_bar--;

				walls_occupied[wall_index] = -2;

				walls[wall_index]->position += out_of_screen;

				sound_source = Sound::play(*open_wall_sample);
				interaction_str = "There is a key here!";

			}
			else if (walls_occupied[wall_index] == 2) {
				crow_bar--;

				walls_occupied[wall_index] = -3;

				walls[wall_index]->position += out_of_screen;

				sound_source = Sound::play(*open_wall_sample);
				interaction_str = "There is a safe here!";

			}
			else if (walls_occupied[wall_index] == 3) {
				crow_bar--;

				walls_occupied[wall_index] = -4;

				walls[wall_index]->position += out_of_screen;

				sound_source = Sound::play(*open_wall_sample);
				interaction_str = "The wall is hollow, but there is nothing in it.";
			}
			else if (walls_occupied[wall_index] == -3) {
				crow_bar--;
				interaction_str = "This safe is too sturdy. I should look for a key.";
			}
			else if (walls_occupied[wall_index] < 0) {
				interaction_str = "I already removed the wall tile here.";
			}

		}
		else if (item_index == 2) {

			if (walls_occupied[wall_index] == -3) {
				walls_occupied[wall_index] = -5;
				sound_source = Sound::play(*use_key_sample);
				safe->position += out_of_screen;
				interaction_str = "The safe opened!";
			}

			else {
				interaction_str = "Why would I do that?";
			}

		}
		else if (item_index == 3) {
			interaction_str = "Why would I do that?";
		}
		else assert(false); // should not reach here
	}
	
	printf("%s \n", interaction_str.c_str());
}

