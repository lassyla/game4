#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>
#include <fstream>


static GLuint vertex_buffer = 0;
static GLuint vertex_buffer_for_color_texture_program = 0;




GLuint sewer_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > sewer_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("sewer.pnct"));
	sewer_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});


Load< Scene > sewer_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("sewer.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = sewer_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = sewer_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});
Load< Sound::Sample > badclick_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("badclick.wav"));
});
Load< Sound::Sample > goodclick_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("goodclick.wav"));
});
Load< Sound::Sample > advance_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("goodclick.wav"));
});
Load< Sound::Sample > playertheme_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("playertheme.wav"));
});
Load< Sound::Sample > oratiotheme_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("oratiotheme.wav"));
});
Load< Sound::Sample > ratcliffetheme_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("ratcliffetheme.wav"));
});

PlayMode::PlayMode() : scene(*sewer_scene) {


	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "oratio") oratio = &transform;
		else if (transform.name == "ratcliffe") ratcliffe = &transform;
	}
	if (oratio == nullptr) throw std::runtime_error("oratio not found.");
	if (ratcliffe == nullptr) throw std::runtime_error("ratcliffe not found.");


	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	std::ifstream in(data_path("../scenes/scenetext.bin"), std::ios::binary);
	
	read_chunk(in, "scns", &textscenes); 
	read_chunk(in, "txts", &textchars); 
	read_chunk(in, "chcs", &choices); 

	// //convert char vector to string https://www.techiedelight.com/convert-vector-chars-std-string/
	// for (char c: textchars) text.append(1, c);
	// std::cout << text; 
	
	current_scene = textscenes[0]; 

	//drawing text code from this tutorial: https://learnopengl.com/In-Practice/Text-Rendering
	

	if (FT_Init_FreeType(&font_library)) throw std::runtime_error("ERROR::FREETYPE: Could not init FreeType Library");

	//convert string to char* https://www.techiedelight.com/convert-std-string-char-cpp/
	if (FT_New_Face(font_library, const_cast<char*>(data_path(font_path).c_str()), 0, &font_face)) throw std::runtime_error("ERROR::FREETYPE: Failed to load font");  
	
    FT_Set_Char_Size(font_face, 0, 1500, 0, 0);
	if (FT_Load_Char(font_face, 'X', FT_LOAD_RENDER)) throw std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph"); 

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
  
	buf = hb_buffer_create();
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_language(buf, hb_language_from_string("en", -1));

	set_scene(0); 

	//the > symbol for your selection
	arrow = hb_buffer_create();
	hb_buffer_add_utf8(arrow, ">", -1, 0, -1);
	hb_buffer_set_direction(arrow, HB_DIRECTION_LTR);
	hb_buffer_set_script(arrow, HB_SCRIPT_LATIN);
	hb_buffer_set_language(arrow, hb_language_from_string("en", -1));
    font = hb_ft_font_create_referenced (font_face);
	hb_shape(font, arrow, NULL, 0);


	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   
	FT_Done_FreeType(font_library);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  


	glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);
	glGenBuffers(1, &vertex_buffer);
	glBindVertexArray(vertex_buffer_for_color_texture_program);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);  
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);   
   
}

PlayMode::~PlayMode() {
	hb_buffer_destroy(buf);
	hb_font_destroy(font);
	FT_Done_Face(font_face);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			if(current_choice_index == 0) {
				Sound::play(*badclick_sample, 1.0f, 0.0f);			
			}
			else {
				current_choice_index--; 
				Sound::play(*goodclick_sample, 1.0f, 0.0f);			
			}
			return true;
		} 
		else if (evt.key.keysym.sym == SDLK_RIGHT) {
			if(current_choice_index == current_scene.num_choices - 1) {
				Sound::play(*badclick_sample, 1.0f, 0.0f);			
			}
			else {
				current_choice_index++; 
				Sound::play(*goodclick_sample, 1.0f, 0.0f);			
			}
			return true;
		} 
		else if (evt.key.keysym.sym == SDLK_RETURN) {
			if(!text_finished){
				text_finished = true; 
				text_length = unsigned int(current_scene.text_length);
			}
			else{
				set_scene(choices[current_scene.choice_start + current_choice_index].scene_id);
				Sound::play(*advance_sample, 1.0f, 0.0f);			
			}

			return true;
		} 
	}
	return false;
}

void PlayMode::set_scene(uint8_t scene_id) {
	//change the sound and display according to specific scenes
	if(scene_id == 15 || scene_id == 2 || scene_id == 0 || scene_id == 27) {
		//show the plain bg 
		oratio->position = hidden_position; 
		ratcliffe->position = hidden_position; 

		//preventing infinite loops
		if(scene_id == 15) {
			if(oratio_route) scene_id = 27; 
			else oratio_route = true; 
		}
		if(scene_id == 1) {
			if(ratcliffe_route) scene_id = 27; 
			else ratcliffe_route = true; 
		}
		if(scene_id == 0) {
			ratcliffe_route = false; 
			oratio_route = false; 
			Sound::stop_all_samples();
			theme_loop = Sound::loop(*playertheme_sample, 1.0f, 0.0f);
		}
		if(scene_id == 27) {
			Sound::stop_all_samples();
			theme_loop = Sound::loop(*playertheme_sample, 1.0f, 0.0f);
		}
	}
	if(scene_id == 16) {
		//show oratio
		Sound::stop_all_samples();
		theme_loop = Sound::loop(*oratiotheme_sample, 1.0f, 0.0f);
		oratio->position = rat_position; 
		ratcliffe->position = hidden_position; 
	}
	if(scene_id == 3) {
		//show ratcliffe 
		Sound::stop_all_samples();
		theme_loop = Sound::loop(*ratcliffetheme_sample, 1.0f, 0.0f);
		oratio->position = hidden_position; 
		ratcliffe->position = rat_position; 

	}

	text_finished = false; 
	text_length = 0;
	text_time_start = 0; 

	current_scene = textscenes[scene_id]; 
	current_choice_index = 0; 

	if (FT_Init_FreeType(&font_library)) throw std::runtime_error("ERROR::FREETYPE: Could not init FreeType Library");
	//convert string to char* https://www.techiedelight.com/convert-std-string-char-cpp/
	if (FT_New_Face(font_library, const_cast<char*>(data_path(font_path).c_str()), 0, &font_face)) throw std::runtime_error("ERROR::FREETYPE: Failed to load font");  
	FT_Set_Char_Size(font_face, 0, 2000, 0, 0);
	if (FT_Load_Char(font_face, 'X', FT_LOAD_RENDER)) throw std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph"); 
	font = hb_ft_font_create_referenced (font_face);

	//main scene text
	hb_buffer_reset(buf); 
	hb_buffer_add_utf8(buf, &textchars[0], -1, int(current_scene.text_start), unsigned int(current_scene.text_length));
	hb_shape(font, buf, NULL, 0);

	//delete previous choices
	for(hb_buffer_t *b : choice_bufs) {
		hb_buffer_destroy(b);
	}
	choice_bufs.clear(); 

	//choice text
	for(size_t i = current_scene.choice_start; i < current_scene.choice_start + current_scene.num_choices; i++) {
		hb_buffer_t *choice_buf = hb_buffer_create();

		hb_buffer_add_utf8(choice_buf, &textchars[0], -1, int(choices[i].text_start), unsigned int(choices[i].text_length));
		hb_buffer_set_direction(choice_buf, HB_DIRECTION_LTR);
		hb_buffer_set_script(choice_buf, HB_SCRIPT_LATIN);
		hb_buffer_set_language(choice_buf, hb_language_from_string("en", -1));

		hb_shape(font, choice_buf, NULL, 0);

		choice_bufs.push_back(choice_buf); 
	}
	

} 

void PlayMode::update(float elapsed) {
	if(!text_finished){
		text_time_start += elapsed; 
		text_length = unsigned int(text_time_start / text_speed); 
		if(text_length >= current_scene.text_length) {
			text_finished = true; 
			text_length = unsigned int(current_scene.text_length);
		}
	}
}

//RenderText base function from https://learnopengl.com/In-Practice/Text-Rendering
//integrated harfbuzz using this tutorial: https://harfbuzz.github.io/ch03s03.html
void PlayMode::render_text(hb_buffer_t *buffer, float width, float x, float y, float scale, int length, glm::vec3 color)
{
    // activate corresponding render state	
    //s.Use();
	float initial_x = x; 

	glDisable(GL_DEPTH_TEST);
	glUseProgram(color_texture_program->program);
    glUniform3f(color_texture_program->Color_vec3, color.x, color.y, color.z);
	glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f);
    glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, &projection[0][0]);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(vertex_buffer_for_color_texture_program);

	unsigned int glyph_count; 
	hb_glyph_info_t *glyph_info    = hb_buffer_get_glyph_infos(buffer, &glyph_count);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_count);
	if(length != -1 && glyph_count > unsigned int(length)) glyph_count = length; 

	for (unsigned int i = 0; i < glyph_count; ++i) {

		//check if the glyph has been loaded yet. if not, load it with this code from https://learnopengl.com/In-Practice/Text-Rendering
		if(Characters.find(glyph_info[i].codepoint) == Characters.end()) {
			if (FT_Init_FreeType(&font_library)) throw std::runtime_error("ERROR::FREETYPE: Could not init FreeType Library");
			if (FT_New_Face(font_library, const_cast<char*>(data_path(font_path).c_str()), 0, &font_face)) throw std::runtime_error("ERROR::FREETYPE: Failed to load font");  
			FT_Set_Char_Size(font_face, 0, 2000, 0, 0);
			if (FT_Load_Char(font_face, 'X', FT_LOAD_RENDER)) throw std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph"); 
			font = hb_ft_font_create_referenced (font_face);

			if (FT_Load_Glyph(font_face, glyph_info[i].codepoint, FT_LOAD_RENDER)) std::runtime_error("ERROR::FREETYTPE: Failed to load Glyph");
			// generate texture
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				font_face->glyph->bitmap.width,
				font_face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				font_face->glyph->bitmap.buffer
			);
			// set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// now store character for later use
			Character character = {
				texture, 
				glm::ivec2(font_face->glyph->bitmap.width, font_face->glyph->bitmap.rows),
				glm::ivec2(font_face->glyph->bitmap_left, font_face->glyph->bitmap_top),
				unsigned int(font_face->glyph->advance.x)
			};
			Characters.insert(std::pair<FT_ULong, Character>(glyph_info[i].codepoint, character));
		}

		Character ch = Characters[glyph_info[i].codepoint];
		//std::cout << "\n" << glyph_info[i].codepoint; 
        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // update vertex_buffer for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };

        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of vertex_buffer memory
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
		//new line if text is too long
		if(x > width + initial_x && glyph_info[i].codepoint == 946){
			x = initial_x; 
			y -= scale* 30.0f; 
		}
	}

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, -0.5f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	render_text(buf, 950.0f, 100.0f, 250.0f, 1.0f, text_length, glm::vec3(0.2, 0.2f, 0.2f));
	
	if(text_finished){
		for(int i = 0; i < choice_bufs.size(); i++) {
			if(i == current_choice_index) 
				render_text(choice_bufs[i], 350.0f, 150.0f + 500.0f * i, 75.0f, 1.0f, -1, glm::vec3(1.0f, 0.4f, 0.4f));
			else 
				render_text(choice_bufs[i], 350.0f, 150.0f + 500.0f * i, 70.0f, 1.0f, -1, glm::vec3(0.2f, 0.2f, 0.2f));
		}
		render_text(arrow, 300.0f, 150.0f + 500.0f * current_choice_index - 12.0f, 75.0f, 1.0f, -1, glm::vec3(1.0f, 0.4f, 0.4f));
	}
	GL_ERRORS();
	
}

