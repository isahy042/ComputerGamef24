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
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *armL = nullptr;
	Scene::Transform *armR = nullptr;
	Scene::Transform *farmL = nullptr;
	Scene::Transform *farmR = nullptr;
	glm::quat armR_base_rotation;
	glm::quat farmR_base_rotation;
	glm::quat armL_base_rotation;
	glm::quat farmL_base_rotation;

	float wobble = 0.f;
	
	//camera:
	Scene::Camera *camera = nullptr;

	void RotateLeftArm() {
	}

	void RotateRightArm() {
	}

	void RotateLeftForeArm() {
	}

	void RotateRightForeArm() {
	}

};
