#include "Mode.hpp"

#include "Scene.hpp"
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

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, one, two, three, four, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	/// starting with getting the transform of key scene components
	static const int total_walls = 32;
	std::vector<int> walls_occupied;
	Scene::Transform* walls[total_walls];
	Scene::Transform* boxes[total_walls];
	Scene::Transform* door;

	Scene::Transform* safe;
	Scene::Transform* key1;
	Scene::Transform* key2;

	Scene::Transform* selector;
	glm::vec3 selector_base_pos;
	
	Scene::Transform* door_selector;
	glm::vec3 door_selector_base_pos;

	int selected_row = 0;
	int selected_col = 0;

	Scene::Transform* item_selector;
	glm::vec3 item_selector_base_pos;
	Scene::Transform* item_holder;

	std::vector<bool> item_list;
	int item_index = 0;
	int crow_bar = 7;

	glm::vec3 out_of_screen = glm::vec3(0.f, -30.f, 0.f);

	// time
	float time = 0.f;
	int increment = 0; // 0.075s

	
	// sound from
	std::shared_ptr< Sound::PlayingSample > sound_source;
	
	//camera:
	Scene::Camera *camera = nullptr;

	bool safe_found = false;
	bool key1_found = false;
	bool key2_found = false;

	bool playing = true;
	bool win = false;
	glm::quat door_base_rotation;
	bool lose = false;
	glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.f);

	// functions
	std::string interaction_str = "Where is this? I need to get out.";
	void interact();

	/* Convert row col to wall index because they don't correspond to each other :(*/
	int get_wall_index();

	/* Use 1-4 to toggle item selection */
	void toggle_item(int item);

	/* Use arrow to toggle wall selection */
	void toggle_wall(bool isLeft, bool isRight, bool isUp, bool isDown);

	


};
