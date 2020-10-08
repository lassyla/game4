#include "ColorTextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< ColorTextureProgram > color_texture_program(LoadTagEarly);

//adapted shader code to follow https://learnopengl.com/In-Practice/Text-Rendering
ColorTextureProgram::ColorTextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"in vec4 Position;\n"
		"out vec2 texCoord;\n"
		"uniform mat4 projection;\n"
		"void main() {\n"
		"   gl_Position = projection * vec4(Position.xy, 0.0, 1.0);\n"
		"	texCoord = Position.zw;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"uniform vec3 textColor;\n"
		"in vec2 texCoord;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"   vec4 sampled = vec4(1.0, 1.0, 1.0, texture(TEX, texCoord).r);\n"
		"   fragColor = vec4(textColor, 1.0) * sampled;\n"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");
	// Color_vec4 = glGetAttribLocation(program, "Color");
	// TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "projection");
	TEX_sampler2D = glGetUniformLocation(program, "TEX");
	Color_vec3 = glGetUniformLocation(program, "textColor"); 
	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	//glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

ColorTextureProgram::~ColorTextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

