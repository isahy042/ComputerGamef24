#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

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
	glm::quat armR_base_rotation;
	glm::quat farmR_base_rotation;
	glm::quat armL_base_rotation;
	glm::quat farmL_base_rotation;
	glm::quat legL_base_rotation;
	glm::quat legR_base_rotation;
	glm::quat clock_base_rotation;
	glm::quat torso_base_rotation;

	float turn_factor = 0.f;
	const float turn_speed = 50.f;
	
	//camera:
	Scene::Camera *camera = nullptr;

	// game count down and score
	bool playing = true;
	const float total_time = 60.f;
	float time = 0.f; // 60 second count down
	float score = 0.f;

	// levels
	float level[18] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	// transforms
	Scene::Transform* transforms[6];
	glm::quat base_rotations[6];

	int channel = 0; // 0 1 2 corresponding to xyz
	int body_part = 0; // 0-5 corresponding to the transforms above

	// boundaries of rotation
	float rotation_lo[18] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	float rotation_hi[18] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	float rotation_min = -100.f;
	float rotation_max = 100.f;

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
			}
			else { // next axis of rotation
				base_rotations[body_part] = transforms[body_part]->rotation;
				channel++;
			}
			turn_factor = 0;
			rotation_direction = 1.f;
		}

	}

	void update_score_meter() {
	}

};
