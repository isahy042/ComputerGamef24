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
	MeshBuffer const *ret = new MeshBuffer(data_path("person.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("person.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
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

PlayMode::PlayMode() : scene(*hexapod_scene) {
	// randomly pick level
	srand((unsigned int)std::time(nullptr));
	level = rand() % total_levels;

	//initialize all body part transformation vector
	for (auto &transform : scene.transforms) {
		if (transform.name == "Arm L") armL = &transform;
		else if (transform.name == "FArm L") farmL = &transform;
		else if (transform.name == "Arm R") armR = &transform;
		else if (transform.name == "FArm R") farmR = &transform;
		else if (transform.name == "Leg L") legL = &transform;
		else if (transform.name == "Leg R") legR = &transform;
		else if (transform.name == "Arm L.001") model_transforms[0] = &transform;
		else if (transform.name == "FArm L.001") model_transforms[1] = &transform;
		else if (transform.name == "Arm R.001") model_transforms[2] = &transform;
		else if (transform.name == "FArm R.001") model_transforms[3] = &transform;
		else if (transform.name == "Leg L.001") model_transforms[4] = &transform;
		else if (transform.name == "Leg R.001") model_transforms[5] = &transform;
		else if (transform.name == "Torso.001") model_transforms[6] = &transform;

		else if (transform.name == "Needle") clock = &transform;
		else if (transform.name == "Torso") torso = &transform;
		else if (transform.name == "Meter") meter = &transform;
	}
	if (armL == nullptr) throw std::runtime_error("Left arm not found.");
	if (farmL == nullptr) throw std::runtime_error("Left forearm not found.");
	if (armR == nullptr) throw std::runtime_error("Right arm not found.");
	if (farmR == nullptr) throw std::runtime_error("Right forearm not found.");
	if (legR == nullptr) throw std::runtime_error("Right Leg not found.");
	if (legL == nullptr) throw std::runtime_error("Left Leg not found.");
	if (clock == nullptr) throw std::runtime_error("Clock not found.");
	if (torso == nullptr) throw std::runtime_error("Torso not found.");
	if (meter == nullptr) throw std::runtime_error("Meter not found.");
	for (int i = 0; i < 6; i++) if (model_transforms[i] == nullptr) throw std::runtime_error("Model not found.");

	armL_base_rotation = armL->rotation;
	farmL_base_rotation = farmL->rotation;
	armR_base_rotation = armR->rotation;
	farmR_base_rotation = farmR->rotation;
	legR_base_rotation = legR->rotation;
	legL_base_rotation = legL->rotation;
	clock_base_rotation = clock->rotation;
	torso_base_rotation = torso->rotation;

	// set position of each body part relative to each other - scene hierarchy
	// attach forearms to arms
	farmL->position = glm::vec3(0.04f, 0.18f, -0.5f);
	farmL->parent = armL;
	farmR->position = glm::vec3(-0.04f, 0.18f, -0.5f);
	farmR->parent = armR;

	// attach everything to torso
	armL->position = glm::vec3(.3f, 0.f, 0.9f);
	armL->parent = torso;
	armR->position = glm::vec3(-.3f, 0.f, 0.9f);
	armR->parent = torso;
	legL->position = glm::vec3(0.18f, 0.f, -0.1f);
	legL->parent = torso;
	legR->position = glm::vec3(-0.18f, 0.f, -0.1f);
	legR->parent = torso;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// assign to transforms and base_rotations 
	transforms[0] = armL;
	transforms[1] = farmL;
	transforms[2] = armR;
	transforms[3] = farmR;
	transforms[4] = legL;
	transforms[5] = legR;
	base_rotations[0] = armL_base_rotation;
	base_rotations[1] = farmL_base_rotation;
	base_rotations[2] = armR_base_rotation;
	base_rotations[3] = farmR_base_rotation;
	base_rotations[4] = legL_base_rotation;
	base_rotations[5] = legR_base_rotation;


	// manipulate level rotation
	for (int i = 0; i < 5; i++) {
		model_transforms[i]->rotation = glm::vec3(levels[level][i * 3], levels[level][i * 3 + 1], levels[level][i * 3 + 2]);
	}

	// set position of each body part relative to each other - scene hierarchy
	// attach forearms to arms
	model_transforms[1]->position = glm::vec3(0.04f, 0.18f, -0.5f);
	model_transforms[1]->parent = model_transforms[0];
	model_transforms[3]->position = glm::vec3(-0.04f, 0.18f, -0.5f);
	model_transforms[3]->parent = model_transforms[2];

	// attach everything to torso
	model_transforms[0]->position = glm::vec3(.3f, 0.f, 0.9f);
	model_transforms[0]->parent = model_transforms[6];
	model_transforms[2]->position = glm::vec3(-.3f, 0.f, 0.9f);
	model_transforms[2]->parent = model_transforms[6];
	model_transforms[4]->position = glm::vec3(0.18f, 0.f, -0.1f);
	model_transforms[4]->parent = model_transforms[6];
	model_transforms[5]->position = glm::vec3(-0.18f, 0.f, -0.1f);
	model_transforms[5]->parent = model_transforms[6];

	model_transforms[6]->scale = glm::vec3(0.5f);
	model_transforms[6]->position += glm::vec3(0,-0.2f, 0);

	

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONUP) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
		return true;
	}
	else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			torso_z += (float)evt.motion.xrel;
			return true;
		}
	}


	return false;
}

void PlayMode::update(float elapsed) {

	// rotate person based on mouse movement
	torso->rotation = torso_base_rotation * glm::angleAxis(
		glm::radians(torso_z),
		glm::vec3(0.0f, 0.0f, 1.0f));

	if (playing && time < total_time) {

		time += elapsed;
		// turn clock needle accordingly
		clock->rotation = clock_base_rotation * glm::angleAxis(
			glm::radians((time/total_time)*360),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		assert(body_part < 6 && channel < 3);

		turn_factor += elapsed * turn_speed * rotation_direction;

		// slowly rotate body part in order
		switch (channel){
			case 0:
				transforms[body_part]->rotation = base_rotations[body_part] * glm::angleAxis(
					glm::radians(turn_factor),
					glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case 1:
				transforms[body_part]->rotation = base_rotations[body_part] * glm::angleAxis(
					glm::radians(turn_factor),
					glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			case 2:
				transforms[body_part]->rotation = base_rotations[body_part] * glm::angleAxis(
					glm::radians(turn_factor),
					glm::vec3(0.0f, 0.0f, 1.0f));
				break;
			default:
				assert(false); // should not reach here.
		}

		if (turn_factor < rotation_min) {
			turn_factor = rotation_min;
			rotation_direction = 1.f;
		}
		else if (turn_factor > rotation_max) {
			turn_factor = rotation_max;
			rotation_direction = -1.f;
		}

		//reset button press counters:
		if (space.pressed) next_rotation();

		//reset button press counters:
		space.pressed = false;
		space.downs = 0;

	}
	//else if (animation) {
	//	// animation

	//}
	else if (meter->position.z < score) {
		float speed = (score) - meter->position.z;
		meter->position += glm::vec3(0.f, 0.f, std::max(0.003f, elapsed * speed * speed));
	}
	
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:

	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.5f,-0.5f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

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

		lines.draw_text("Match the pose of the displayed doll!",
			glm::vec3(-aspect + 0.09f * H, 0.85 - 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));

		lines.draw_text("Use [Space] to pause the rotation. [Drag] left and right to spin the person",
			glm::vec3(-aspect + 0.09f * H, 0.75 - 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	}

	
}
