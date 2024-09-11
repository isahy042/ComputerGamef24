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
	glm::quat armR_base_rotation;
	glm::quat farmR_base_rotation;
	glm::quat armL_base_rotation;
	glm::quat farmL_base_rotation;
	glm::quat legL_base_rotation;
	glm::quat legR_base_rotation;
	glm::quat clock_base_rotation;
	glm::quat torso_base_rotation;

	float turn_factor = 0.f;
	const float turn_speed = 120.f;

	float torso_z = 0.f;
	
	//camera:
	Scene::Camera *camera = nullptr;

	// game count down and score
	bool playing = true;
	bool animation = true;
	float total_time = 60.f;// 60 second count down
	float time = 0.f; 
	float score = 2.f;
	
	static const int total_levels = 1;
	int level = 0;
	// levels
	float model_pose[1][18] = { {
		-0.084202f, -1.3f, 0.073796f,
		0.262006f, 0.102229f, -0.348818f,
		-1.6f, 0.002514f, 0.001846f,
		0.217125f, 0.1f, 0.009435f,
		1.25f, 0.1f, -0.124795f,
		0.981199f, 0.192127f, -0.003523f}
	};

	float levels[1][18] = { {
		-0.084202f, -0.654961f, 0.073796f,
		0.262006f, 0.102229f, -0.348818f,
		-0.805994f, 0.002514f, 0.001846f,
		0.217125f, 0.042378f, 0.009435f,
		0.566970f, 0.087403f, -0.124795f,
		0.981199f, 0.192127f, -0.003523f}
	};

	// transforms
	Scene::Transform* transforms[6];
	glm::quat base_rotations[6];
	Scene::Transform* model_transforms[7];

	int channel = 0; // 0 1 2 corresponding to xyz
	int body_part = 0; // 0-5 corresponding to the transforms above

	// boundaries of rotation
	float rotation_lo[18] = {
		-150,-130,-100,-70,-120,-100,
		-180,0,0,-40,-120,-180,
		-80,-70,-30,-80,-20,-30
	};

	float rotation_hi[18] = {
		55,0,0,40,10,100,
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
			base_rotations[body_part] = transforms[body_part]->rotation;
			// skipping the following 
			if ((body_part == 0 || body_part == 2 || body_part == 3) && channel == 1) {
				body_part++;
				channel = 0;
			}
			else if ((body_part == 1 || body_part == 2 || body_part == 4 || body_part == 5) && channel == 0) {
				channel = 2;
			}
			else if (channel == 2) { // next body part
				body_part++;
				channel = 0;
			}
			else { // next axis of rotation
				channel++;
			}

			rotation_min = rotation_lo[body_part * 3 + channel];
			rotation_max = rotation_hi[body_part * 3 + channel];

			turn_factor = 0;
			rotation_direction = 1.f;
		}

	}

	void update_score_meter() {
		
		total_time = time;
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 3; j++) {
				float f = (float)(levels[level][i * 3 + j] - transforms[i]->rotation[j]);
				score -= (f * f);
			}
		}
		
		score += 1.f; // normalize from -1 to 1 to 0 to 2
		score = (score < 0.1f) ? 0.1f : score;
		score = (score > 2.f) ? 2.f : score;

		score += 0.5f; // adding mesh offset

		
	}

};
