#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include <glm/gtx/norm.hpp>

void Player::Controls::send_controls_message(Connection *connection_) const {
	assert(connection_);
	auto &connection = *connection_;

	uint32_t size = 5;
	connection.send(Message::C2S_Controls);
	connection.send(uint8_t(size));
	connection.send(uint8_t(size >> 8));
	connection.send(uint8_t(size >> 16));

	auto send_button = [&](Button const &b) {
		if (b.downs & 0x80) {
			std::cerr << "Wow, you are really good at pressing buttons!" << std::endl;
		}
		connection.send(uint8_t( (b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f) ) );
	};

	send_button(left);
	send_button(right);
	send_button(up);
	send_button(down);
	send_button(jump);
}

bool Player::Controls::recv_controls_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;

	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::C2S_Controls)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	if (size != 5) throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 5!");
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	auto recv_button = [](uint8_t byte, Button *button) {
		button->pressed = (byte & 0x80);
		uint32_t d = uint32_t(button->downs) + uint32_t(byte & 0x7f);
		if (d > 255) {
			std::cerr << "got a whole lot of downs" << std::endl;
			d = 255;
		}
		button->downs = uint8_t(d);
	};

	recv_button(recv_buffer[4+0], &left);
	recv_button(recv_buffer[4+1], &right);
	recv_button(recv_buffer[4+2], &up);
	recv_button(recv_buffer[4+3], &down);
	recv_button(recv_buffer[4+4], &jump);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}


//-----------------------------------------

Game::Game() : mt(0x15466666) {
	// initialize list of NPCs
	for (int i = 0; i < 20; i++) {
		NPCs.emplace_back();
		NPC& n = NPCs.back();
		// random spawn point
		n.position.x = glm::mix(ArenaMin.x + 2.0f * PlayerRadius, ArenaMax.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
		n.position.y = glm::mix(ArenaMin.y + 2.0f * PlayerRadius, ArenaMax.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));

	}

	// initialize gem position
	float y = -0.5f;
	for (int i = 0; i < 3; i++) {
		gem_pos.push_back(glm::vec2(-0.35f, y));
		gem_pos.push_back(glm::vec2(0.35f, y));
		y += 0.6f;
	}
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	//random point in the middle area of the arena:
	player.position.x = glm::mix(ArenaMin.x + 2.0f * PlayerRadius, ArenaMax.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
	player.position.y = glm::mix(ArenaMin.y + 2.0f * PlayerRadius, ArenaMax.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));

	do {
		player.color.r = mt() / float(mt.max());
		player.color.g = mt() / float(mt.max());
		player.color.b = mt() / float(mt.max());
	} while (player.color == glm::vec3(0.0f));
	player.color = glm::normalize(player.color);

	player.index = next_player_number;
	player.name = "Player " + std::to_string(next_player_number++);

	return &player;
}

void Game::remove_player(Player *player) {
	bool found = false;
	for (auto pi = players.begin(); pi != players.end(); ++pi) {
		if (&*pi == player) {
			players.erase(pi);
			found = true;
			break;
		}
	}
	assert(found);
}

void Game::update(float elapsed) {

	//player position/velocity update:

	time += elapsed;
	
	if (time >= 1.5f) laser = false;

	if (time >= 2.f) {
		for (auto& p : players) {
			if (p.index == 1 && p.controls.jump.pressed && ammo > 0) {
				ammo -= 1;
				laser = true;
				time = 0.f;
			}
		}
	}

	for (auto &p : players) {
		glm::vec2 dir = glm::vec2(0.0f, 0.0f);
		if (p.controls.left.pressed) dir.x -= 1.0f;
		if (p.controls.right.pressed) dir.x += 1.0f;
		if (p.controls.down.pressed) dir.y -= 1.0f;
		if (p.controls.up.pressed) dir.y += 1.0f;

		for (int g = 0; g < 6; g++) {
			if (p.index > 1) {
				// kill player if it is 0.1f away from the gem
				if (laser && glm::distance(p.position, gem_pos[g]) < 0.17f) {
					if (p.active) ammo+=3; // gain 3 ammo from killing a player
					p.active = false;

				}
				else if (glm::distance(p.position, gem_pos[g]) < 0.07f) {
					gem[g] = false; // steal!
				}
			}
			
		}

		if (dir == glm::vec2(0.0f)) {
			p.velocity = glm::vec2(0.0f, 0.0f);
		} else {
			//inputs: tween velocity to target direction
			p.velocity = glm::normalize(dir);
		}

		// clamp within arena
		p.position += p.velocity * elapsed * 0.3f;

		//player/arena collisions:
		if (p.position.x < ArenaMin.x + PlayerRadius) {
			p.position.x = ArenaMin.x + PlayerRadius;
			p.velocity.x = std::abs(p.velocity.x);
		}
		if (p.position.x > ArenaMax.x - PlayerRadius) {
			p.position.x = ArenaMax.x - PlayerRadius;
			p.velocity.x = -std::abs(p.velocity.x);
		}
		if (p.position.y < ArenaMin.y + PlayerRadius) {
			p.position.y = ArenaMin.y + PlayerRadius;
			p.velocity.y = std::abs(p.velocity.y);
		}
		if (p.position.y > ArenaMax.y - PlayerRadius) {
			p.position.y = ArenaMax.y - PlayerRadius;
			p.velocity.y = -std::abs(p.velocity.y);
		}

		//reset 'downs' since controls have been handled:
		p.controls.left.downs = 0;
		p.controls.right.downs = 0;
		p.controls.up.downs = 0;
		p.controls.down.downs = 0;
		p.controls.jump.downs = 0;
	}

	//NPC position/velocity update:
	for (auto& npc : NPCs) {
		
		npc.time -= elapsed;
		// get new direction and new time
		if (npc.time <= 0.f) {
			int temp_time = (rand() % 5) + 1;
			
			npc.time = ((float)temp_time) / 10.f; // 0.1 to 2.5s

			// whether to stay idle
			npc.dir = glm::vec2(0.0f, 0.0f);
			if ((rand() % 20) != 0) {
				npc.dir = glm::vec2((rand() % 3) - 1, (rand() % 3) - 1);
			}
		}

		if (npc.dir == glm::vec2(0.0f)) {
			//no inputs: just drift to a stop
			npc.velocity = glm::vec2(0.0f, 0.0f);
		}
		else {
			//inputs: tween velocity to target direction
			npc.velocity = glm::normalize(npc.dir);
		}

		// clamp within arena
		npc.position += npc.velocity * elapsed * 0.3f;

		//player/arena collisions:
		if (npc.position.x < ArenaMin.x + PlayerRadius) {
			npc.position.x = ArenaMin.x + PlayerRadius;
			npc.velocity.x = std::abs(npc.velocity.x);
		}
		if (npc.position.x > ArenaMax.x - PlayerRadius) {
			npc.position.x = ArenaMax.x - PlayerRadius;
			npc.velocity.x = -std::abs(npc.velocity.x);
		}
		if (npc.position.y < ArenaMin.y + PlayerRadius) {
			npc.position.y = ArenaMin.y + PlayerRadius;
			npc.velocity.y = std::abs(npc.velocity.y);
		}
		if (npc.position.y > ArenaMax.y - PlayerRadius) {
			npc.position.y = ArenaMax.y - PlayerRadius;
			npc.velocity.y = -std::abs(npc.velocity.y);
		}

	}

}

void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(Message::S2C_State);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer


	//send player info helper:
	auto send_player = [&](Player const &player) {
		connection.send(player.position);
		connection.send(player.velocity);
		connection.send(player.color);
		connection.send(player.index);
		connection.send(player.active);
	
		//NOTE: can't just 'send(name)' because player.name is not plain-old-data type.
		//effectively: truncates player name to 255 chars
		uint8_t len = uint8_t(std::min< size_t >(255, player.name.size()));
		connection.send(len);
		connection.send_buffer.insert(connection.send_buffer.end(), player.name.begin(), player.name.begin() + len);
	};

	//player count:
	connection.send(uint8_t(players.size()));
	if (connection_player) send_player(*connection_player);
	for (auto const &player : players) {
		if (&player == connection_player) continue;
		send_player(player);
	}

	// send npcs
	auto send_npc = [&](NPC const& npc) {
		connection.send(npc.position);
		connection.send(npc.color);
		};
	connection.send(uint8_t(NPCs.size()));
	for (auto const& NPC : NPCs) {
		send_npc(NPC);
	}

	// send ammo info
	connection.send(ammo);
	connection.send(laser);

	// gem stolen info
	connection.send(gem);


	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
}

bool Game::recv_state_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;

	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::S2C_State)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	uint32_t at = 0;
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	//copy bytes from buffer and advance position:
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	// read player data
	players.clear();
	uint8_t player_count;
	read(&player_count);
	for (uint8_t i = 0; i < player_count; ++i) {
		players.emplace_back();
		Player &player = players.back();
		read(&player.position);
		read(&player.velocity);
		read(&player.color);
		read(&player.index);
		read(&player.active);
		uint8_t name_len;
		read(&name_len);
		//n.b. would probably be more efficient to directly copy from recv_buffer, but I think this is clearer:
		player.name = "";
		for (uint8_t n = 0; n < name_len; ++n) {
			char c;
			read(&c);
			player.name += c;
		}
	}

	// read NPC data
	NPCs.clear();
	uint8_t npc_count;
	read(&npc_count); // always 20
	for (uint8_t i = 0; i < npc_count; ++i) {
		NPCs.emplace_back();
		NPC& npc = NPCs.back();
		read(&npc.position);
		read(&npc.color);
	}

	// ammo info
	read(&ammo);
	read(&laser);

	// gem stolen info
	read(&gem);

	if (at != size) throw std::runtime_error("Trailing data in state message.");

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}
