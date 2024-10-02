#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Sound.hpp"

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

	void pv3(glm::vec3 const v, std::string const s) const {
		printf(s.c_str()); printf(": %f %f %f \n", v.x, v.y, v.z);
	}

	Scene::Transform* spheres[3];
	Scene::Transform* sticks[3];
	glm::quat sticks_rot[3];
	std::string finalstr = "";
	float highScore = -1.f;
	std::string hs = ""; // high score

	// sound from
	std::shared_ptr< Sound::PlayingSample > sound_source;

	//----- game state -----

	int playing = 1; // 0 = win, -1 = lose
	float seconds = 0.f;
	float speed = 0.f;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} r, down, up, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//player info:
	struct Player {
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} player;
	

	// helper function
	glm::vec3 ChildWorldPos(const glm::vec3 parentPos, const glm::vec3 childPos, const glm::quat parentRot) {
		// rotate child location
		glm::vec3 rotatedChild = parentRot * childPos;
		// offset
		return parentPos + rotatedChild;
	}

	void ResetGame() {
		seconds = 0;
		playing = 1;
		player.transform->position = glm::vec3(0.f);
		speed = 0.0f;
	}
};
