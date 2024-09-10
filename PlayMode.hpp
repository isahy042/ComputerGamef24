#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <algorithm>
#include <deque>
#include <string>

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
	} space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *armL = nullptr;
	Scene::Transform *armR = nullptr;
	Scene::Transform *legR = nullptr;
	Scene::Transform *legL = nullptr;
	Scene::Transform *farmL = nullptr;
	Scene::Transform *farmR = nullptr;
	Scene::Transform *clock = nullptr;
	Scene::Transform *torso = nullptr;
	Scene::Transform *meter = nullptr;
	Scene::Transform *level_transform = nullptr;
	glm::quat armR_base_rotation;
	glm::quat farmR_base_rotation;
	glm::quat armL_base_rotation;
	glm::quat farmL_base_rotation;
	glm::quat legL_base_rotation;
	glm::quat legR_base_rotation;
	glm::quat clock_base_rotation;
	glm::quat torso_base_rotation;

	float turn_factor = 0.f;
	const float turn_speed = 100.f;
	
	//camera:
	Scene::Camera *camera = nullptr;

	// game count down and score
	bool playing = true;
	const float total_time = 60.f;
	float time = 0.f; // 60 second count down
	
	static const int total_levels = 1;
	int level = 0;
	// levels
	float levels[total_levels][18] = { {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} // the stand still
	};

	// transforms
	Scene::Transform* transforms[6];
	glm::quat base_rotations[6];

	int channel = 0; // 0 1 2 corresponding to xyz
	int body_part = 0; // 0-5 corresponding to the transforms above

	// boundaries of rotation
	float rotation_lo[18] = {
		-180,-160,-100,-70,-120,-180,
		-180,0,0,-40,-120,-180,
		-80,-70,-30,-80,-20,-30
	};

	float rotation_hi[18] = {
		55,0,0,40,10,180,
		0,160,100,70,10,180,
		80,20,30,80,70,30
	};

	float rotation_min = rotation_lo[0];
	float rotation_max = rotation_hi[0];

	float rotation_direction = -1.f;

	// record the previous thing rotated, switch to the next thing to be rotated
	void next_rotation() {
		
		if (body_part == 5 && channel == 2) { // last
			playing = false;
			update_score_meter();
		}
		else {
			if (channel == 2) { // next body part
				base_rotations[body_part] = transforms[body_part]->rotation;
				body_part++;
				channel = 0;
				rotation_min = rotation_lo[body_part * 3];
				rotation_max = rotation_hi[body_part * 3];
			}
			else { // next axis of rotation
				base_rotations[body_part] = transforms[body_part]->rotation;
				channel++;
				rotation_min = rotation_lo[body_part * 3 + channel];
				rotation_max = rotation_hi[body_part * 3 + channel];
			}
			turn_factor = 0;
			rotation_direction = 1.f;
		}

	}

	void update_score_meter() {
		float score = 100;
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 3; j++) {
				score -= 3 * abs(levels[level][i * 3 + j] - floor(transforms[i]->rotation[j]));
			}
		}

		meter->position += glm::vec3(0.f, 0.f, score / 50.f);
	}

};
