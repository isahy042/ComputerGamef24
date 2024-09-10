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

	Scene::Transform* transforms[6] = { armL, farmL, armR, farmR, legL, legR };
	int channel = 0; // 0 1 2 corresponding to xyz
	int body_part = 0; // 0-5 corresponding to the transforms above

	// boundaries of rotation
	float rotation_lo[18] = {

	};
	float rotation_hi[18] = {

	};

	float level[18] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	const float total_time = 60.f;
	float time = 60.f; // 60 second count down
	float score = 0.f;

	float rotation_direction = -1.f;
	
	Scene::Transform* curr_transform = nullptr;
	glm::quat curr_base_rotation = nullptr;
	// record the previous thing rotated, switch to the next thing to be rotated
	void next_rotation() {
	}
	

};
