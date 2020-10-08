#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "TextScene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <map>
#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H  

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	virtual void render_text(hb_buffer_t *buffer, float width, float x, float y, float scale, int length, glm::vec3 color);
	virtual void set_scene(uint8_t scene_id); 
	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *oratio = nullptr;
	Scene::Transform *ratcliffe = nullptr;
	glm::vec3 rat_position = glm::vec3(0.0f, 0.0f, 4.0f); 
	glm::vec3 hidden_position = glm::vec3(0.0f, -30.0f, 0.0f); 
	

	float text_speed = .025f; 
	unsigned int text_length = 0; 
	float text_time_start = 0; 
	bool text_finished = false;

	bool ratcliffe_route = false; 
	bool oratio_route = false; 

	size_t current_choice_index = 0; 

	glm::vec3 get_leg_tip_position();

	std::shared_ptr< Sound::PlayingSample > theme_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

	//text scene assets 
	std::vector <TextScene> textscenes; 
	TextScene current_scene; 
    std::vector <Choice> choices; 
	//std::string text; 
	std::vector<char> textchars; 

	//font character struct from https://learnopengl.com/In-Practice/Text-Rendering
	struct Character {
		unsigned int TextureID;  // ID handle of the glyph texture
		glm::ivec2   Size;       // Size of glyph
		glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
		unsigned int Advance;    // Offset to advance to next glyph
	};

	std::map<FT_ULong, Character> Characters;
	hb_buffer_t *buf;
	hb_buffer_t *arrow;
	std::vector <hb_buffer_t *> choice_bufs; 
	hb_font_t *font;
	FT_Face font_face; 
	FT_Library font_library;
	std::string font_path = "../fonts/SansitaSwashed-VariableFont_wght.ttf";

};
