#include <string>
#include <vector>

struct TextScene {
    //std::string text; 
    uint8_t id; 
    size_t num_choices; 
    size_t choice_start; 
    //std::vector <uint8_t> choice_id; 
    //std::vector <std::string> choice_text;
    size_t text_start; 
    size_t text_length; 
};

struct Choice {
    uint8_t scene_id; 
    size_t text_start; 
    size_t text_length; 
};