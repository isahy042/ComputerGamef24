#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <random>

GLuint phonebank_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > phonebank_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("bicycle.pnct"));
	phonebank_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > phonebank_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("bicycle.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = phonebank_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = phonebank_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("bicycle.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

Load< Sound::Sample > ring_sound(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("Ring.wav"));
	});

PlayMode::PlayMode() : scene(*phonebank_scene) {
	//create a player transform:
	scene.transforms.emplace_back();
	player.transform = &scene.transforms.back();

	//create a player camera attached to a child of the player transform:
	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	player.camera = &scene.cameras.back();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;
	player.camera->transform->parent = player.transform;

	//player's eyes are 1.3 units above the ground:
	player.camera->transform->position = glm::vec3(0.0f, 0.0f, 1.3f);

	//rotate camera facing direction (-z) to player facing direction (+y):
	player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	player.transform->rotation = glm::quat(-0.7263f, 0.0f, 0.0f, 0.687300f);

	//start player walking at nearest walk point:
	player.at = walkmesh->nearest_walk_point(player.transform->position);

	for (auto& transform : scene.transforms) {
		if (transform.name == "Sphere") spheres[0] = &transform;
		else if (transform.name == "Sphere1") spheres[1] = &transform;
		else if (transform.name == "Sphere2") spheres[2] = &transform;
		else if (transform.name == "Stick") sticks[0] = &transform;
		else if (transform.name == "Stick1") sticks[1] = &transform;
		else if (transform.name == "Stick2") sticks[2] = &transform;
		else if (transform.name == "Player") { 
			// attach bicycle to player
			transform.position += glm::vec3(0.f,0.4f, 0.8f);
			transform.rotation *= glm::angleAxis(
				glm::radians(90.f),
				glm::vec3(0.0f, 0.0f, 1.0f));
			transform.parent = player.transform;
		}
	}

	for (int i = 0; i < 3; i++) {
		sticks_rot[i] = sticks[i]->rotation;
		spheres[i]->parent = sticks[i];
		spheres[i]->position -= glm::vec3(0.f, 0.f, 2.f);
	}

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			r.downs += 1;
			r.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_r) {
			r.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			glm::vec3 upDir = walkmesh->to_world_smooth_normal(player.at);
			player.transform->rotation = glm::angleAxis(-motion.x * player.camera->fovy, upDir) * player.transform->rotation;

			float pitch = glm::pitch(player.camera->transform->rotation);
			pitch += motion.y * player.camera->fovy;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			pitch = std::min(pitch, 0.95f * 3.1415926f);
			pitch = std::max(pitch, 0.05f * 3.1415926f);
			player.camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	//rotate balls
	if (space.pressed) sound_source = Sound::play(*ring_sound);
	if (playing > 0) {
	seconds += elapsed;
	for (int i = 0; i < 3; i++) {
		sticks[i]->rotation = sticks_rot[i] * glm::angleAxis(
			glm::radians(sin((seconds + i)) * 40.f),
			glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 ballPos = ChildWorldPos(sticks[i]->position, spheres[i]->position, sticks[i]->rotation);
		if (glm::distance(ballPos, player.transform->position + glm::vec3(0.f,0.f,1.6f)) < 1.2f) { 
			playing = -1; 
			return;
		}
	}

	//player biking
	{
		if (down.pressed && !up.pressed) speed -= elapsed;
		if (!down.pressed && up.pressed) speed += elapsed;

		if (speed != 0) {
			speed = (speed > 1.f) ? 1.f : speed;
			speed = (speed < -1.f) ? -1.f : speed;
			speed = (speed > 0) ? std::max(0.f, speed - elapsed * 0.1f) : std::min(0.f, speed + elapsed * 0.1f);
		}
		
		//get move in world coordinate system:
		glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(0.0f, speed, 0.0f, 0.0f);
		//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:
		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			walkmesh->walk_in_triangle(player.at, remain, &end, &time);
			player.at = end;
			if (time == 1.0f) {
				//finished within triangle:
				remain = glm::vec3(0.0f);
				break;
			}
			//some step remains:
			remain *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;

			if (walkmesh->cross_edge(player.at, &end, &rotation)) {
				//stepped to a new triangle:
				player.at = end;
				//rotate step to follow surface:
				remain = rotation * remain;

			}
			else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const& a = walkmesh->vertices[player.at.indices.x];
				glm::vec3 const& b = walkmesh->vertices[player.at.indices.y];
				glm::vec3 const& c = walkmesh->vertices[player.at.indices.z];
				glm::vec3 along = glm::normalize(b - a);
				glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain += (-1.25f * d) * in;
				}
				else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain += 0.01f * d * in;
				}
			}
		}

		if (remain != glm::vec3(0.0f)) {
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		player.transform->position = walkmesh->to_world_point(player.at);

		{ //update player's rotation to respect local (smooth) up-vector:

			glm::quat adjust = glm::rotation(
				player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
			);
			player.transform->rotation = glm::normalize(adjust * player.transform->rotation);
		}

		if (player.transform->position.x >= 62.8f) {
			playing = 0;
		}

		/*
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
		*/
	}
	}
	else if (playing == -1) {
		// lose
		finalstr = "YOU GOT HIT BY A HUGE HEAVY METAL BALL :(";
		hs = (highScore < 0) ? "High Score: Death via Metal Ball" : hs;
		if (r.pressed) {
			ResetGame();
			player.at = walkmesh->nearest_walk_point(player.transform->position);
		}
	}
	else if (playing == 0) {
		// win
		finalstr = "SUCCESS!!!";
		highScore = (highScore < 0) ? seconds : std::min(highScore, seconds);
		hs = "High Score: " + std::to_string(highScore);
		if (r.pressed) {
			ResetGame();
			player.at = walkmesh->nearest_walk_point(player.transform->position);
		}
	}
	//reset button press counters:
	r.downs = 0;
	up.downs = 0;
	down.downs = 0;
	space.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

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

	scene.draw(*player.camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

	
		lines.draw_text("Use Mouse to adjust orientation, [W] [S] to pedal front and back",
			glm::vec3(-aspect + 0.1f, -1.0 + 0.05f, 0.0),
			glm::vec3(0.1f, 0.0f, 0.0f), glm::vec3(0.0f, 0.1f, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(hs.c_str(),
			glm::vec3(-aspect + 0.1f, -1.0 + 0.18f, 0.0),
			glm::vec3(0.1f, 0.0f, 0.0f), glm::vec3(0.0f, 0.1f, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));


		if (playing == 0) {
			lines.draw_text(finalstr.c_str(),
				glm::vec3(-aspect/2.f , 0.f ,0.f),
				glm::vec3(.5f, 0.0f, 0.0f), glm::vec3(0.0f, .5f, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			lines.draw_text("[R] to restart",
				glm::vec3(-aspect + 0.1f, -1.0 + 0.3f, 0.0),
				glm::vec3(0.08f, 0.0f, 0.0f), glm::vec3(0.0f, 0.08f, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		} else if (playing == -1) {
			lines.draw_text(finalstr.c_str(),
				glm::vec3(-aspect + 0.1f, 0.f, 0.f),
				glm::vec3(.2f, 0.0f, 0.0f), glm::vec3(0.0f, .2f, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			lines.draw_text("[R] to restart",
				glm::vec3(-aspect + 0.1f, -1.0 + 0.3f, 0.0),
				glm::vec3(0.08f, 0.0f, 0.0f), glm::vec3(0.0f, 0.08f, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
	}
	GL_ERRORS();
}
