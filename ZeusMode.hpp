//
//  ZeusMode.hpp
//  Refercing the PongMode.cpp
//
//  Created by owen ou on 2021/9/4.
//

#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct ZeusMode : Mode {
    ZeusMode();
    virtual ~ZeusMode();
    
    //functions called by main_loop
    virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    virtual void update(float elapsed) override;
    virtual void draw(glm::uvec2 const &drawable_size) override;
    
    //----- game state -----
    //TODO: fill in game object states, size, scores
    glm::vec2 scene_radius = glm::vec2(7.0f, 5.0f);             //size of the scene
    glm::vec2 cloud_radius = glm::vec2(1.0f, 0.2f);             //size of Zeus' cloud
    glm::vec2 bullet_radius = glm::vec2(0.2f, 0.2f);            //size of bullet
    
    glm::vec2 cloud = glm::vec2(0.0f, scene_radius.y - 1.0f);   //position of cloud, center of rectangle
    
    // TODO: need vector to store multiple bullets
//    std::deque< glm::vec3 > bullets;                          //(x,y,age), bullet position 2d
//    std::deque< glm::vec3 > bullet_velocity;                  //bullet velocity, static at beginning
    glm::vec2 bullet = glm::vec2(0.0f, cloud.y);                //bullet position 2d //TODO: place at center of cloud
//    glm::vec3 bullet = glm::vec3(0.0f, cloud.y, 0.0f);        //(x,y,age)   //TODO: include age
    glm::vec2 bullet_velocity = glm::vec2(0.0f, 0.0f);          //bullet velocity, static at beginning
    glm::vec2 gravity = glm::vec2(0.0f, -9.8f);                  //gravitational acceleration
    float bullet_cd = 0.5f;                                     //bullet cool down  //TODO: may not need this
    bool bullet_fired = false;                                  //bullet fire state
    //TODO: may need to add bullet life
    
    uint32_t score = 0;
    
    std::vector< glm::vec2 > buildings;                          //buildings center
    std::vector< glm::vec2 > buildings_radius;                   //size of each building
    const float buildings_width = 1.0f;                     //fixed width of each building
    const int max_buildings = 7;                            //max number of buildings
    float grow_rate = 0.1f;                                 //building growing rate
    float spawn_cd_min = 0.5f;                          //new building min spawn cool down
    float spawn_cd_max = 2.0f;                          //new building max spawn cool down
    float grow_cd = 1.0f;                               //building growing cool down
    float grow_update = 0.0f;                           //update last grow time
    
    float ai_offset = 0.0f;
    float ai_offset_update = 0.0f;
    
    //----- pretty gradient trails -----
    float trail_length = 0.8f;              //original: 1.3f, make shorter
    std::deque< glm::vec3 > bullet_trail;   //stores (x,y,age), oldest elements first
    
    //----- opengl assets / helpers ------
    
    //draw functions will work on vectors of vertices, defined as follows:
    struct Vertex {
        Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
            Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
        glm::vec3 Position;
        glm::u8vec4 Color;
        glm::vec2 TexCoord;
    };
    static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "Zeus::Vertex should be packed");
    
    //Shader program that draws transformed, vertices tinted with vertex colors:
    ColorTextureProgram color_texture_program;

    //Buffer used to hold vertex data during drawing:
    GLuint vertex_buffer = 0;

    //Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
    GLuint vertex_buffer_for_color_texture_program = 0;

    //Solid white texture:
    GLuint white_tex = 0;

    //matrix that maps from clip coordinates to court-space coordinates:
    glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
    // computed in draw() as the inverse of OBJECT_TO_CLIP
    // (stored here so that the mouse handling code can use it to position the paddle)
};
