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
	//initialize all body part transformation vector
	for (auto &transform : scene.transforms) {
		if (transform.name == "Arm L") armL = &transform;
		else if (transform.name == "FArm L") farmL = &transform;
		else if (transform.name == "Arm R") armR = &transform;
		else if (transform.name == "FArm R") farmR = &transform;
		else if (transform.name == "Leg L") legL = &transform;
		else if (transform.name == "Leg R") legR = &transform;
		else if (transform.name == "Needle") clock = &transform;
		else if (transform.name == "Torso") torso = &transform;
	}
	if (armL == nullptr) throw std::runtime_error("Left arm not found.");
	if (farmL == nullptr) throw std::runtime_error("Left forearm not found.");
	if (armR == nullptr) throw std::runtime_error("Right arm not found.");
	if (farmR == nullptr) throw std::runtime_error("Right forearm not found.");
	if (legR == nullptr) throw std::runtime_error("Right Leg not found.");
	if (legL == nullptr) throw std::runtime_error("Left Leg not found.");
	if (clock == nullptr) throw std::runtime_error("Clock not found.");
	if (torso == nullptr) throw std::runtime_error("Torso not found.");

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
	//else if (evt.type == SDL_MOUSEBUTTONDOWN) {
	//	if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
	//		SDL_SetRelativeMouseMode(SDL_TRUE);
	//		return true;
	//	}
	//} else if (evt.type == SDL_MOUSEMOTION) {
	//	if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
	//		/*glm::vec2 motion = glm::vec2(
	//			evt.motion.xrel / float(window_size.y),
	//			-evt.motion.yrel / float(window_size.y)
	//		);
	//		camera->transform->rotation = glm::normalize(
	//			camera->transform->rotation
	//			* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
	//			* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
	//		);*/
	//		return true;
	//	}
	//}


	return false;
}

void PlayMode::update(float elapsed) {

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

		//if (turn_factor < rotation_min) {
		//	turn_factor = rotation_min;
		//	rotation_direction = 1.f;
		//}
		//else if (turn_factor > rotation_max) {
		//	turn_factor = rotation_max;
		//	rotation_direction = -1.f;
		//}

		//upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		//	glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		//	glm::vec3(0.0f, 0.0f, 1.0f)
		//);
		//lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		//	glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		//	glm::vec3(0.0f, 0.0f, 1.0f)
		//);

		////move camera:
		//{

		//	//combine inputs into a move:
		//	constexpr float PlayerSpeed = 30.0f;
		//	glm::vec2 move = glm::vec2(0.0f);
		//	if (left.pressed && !right.pressed) move.x =-1.0f;
		//	if (!left.pressed && right.pressed) move.x = 1.0f;
		//	if (down.pressed && !up.pressed) move.y =-1.0f;
		//	if (!down.pressed && up.pressed) move.y = 1.0f;

		//	//make it so that moving diagonally doesn't go faster:
		//	if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//	glm::mat4x3 frame = camera->transform->make_local_to_parent();
		//	glm::vec3 frame_right = frame[0];
		//	//glm::vec3 up = frame[1];
		//	glm::vec3 frame_forward = -frame[2];

		//	camera->transform->position += move.x * frame_right + move.y * frame_forward;
		//}

		//reset button press counters:
		if (space.pressed) next_rotation();


		//reset button press counters:
		space.pressed = false;
		space.downs = 0;

	}
	
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
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

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	
}
