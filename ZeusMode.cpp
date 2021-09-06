//
//  ZeusMode.cpp
//  Refercing the PongMode.cpp
//
//  Created by owen ou on 2021/9/4.
//

#include "ZeusMode.hpp"
//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

#include <assert.h> //prevent error

ZeusMode::ZeusMode() {
    //set up trail as if bullet has been here for 'forever':
    bullet_trail.clear();
    bullet_trail.emplace_back(bullet, trail_length);
    bullet_trail.emplace_back(bullet, 0.0f);
    
    //----- allocate OpenGL resources -----
    { //vertex buffer:
        glGenBuffers(1, &vertex_buffer);
        //for now, buffer will be un-filled.

        GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
    }
    
    { //vertex array mapping buffer for color_texture_program:
        //ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
        glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

        //set vertex_buffer_for_color_texture_program as the current vertex array object:
        glBindVertexArray(vertex_buffer_for_color_texture_program);

        //set vertex_buffer as the source of glVertexAttribPointer() commands:
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

        //set up the vertex array object to describe arrays of PongMode::Vertex:
        glVertexAttribPointer(
            color_texture_program.Position_vec4, //attribute
            3, //size
            GL_FLOAT, //type
            GL_FALSE, //normalized
            sizeof(Vertex), //stride
            (GLbyte *)0 + 0 //offset
        );
        glEnableVertexAttribArray(color_texture_program.Position_vec4);
        //[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

        glVertexAttribPointer(
            color_texture_program.Color_vec4, //attribute
            4, //size
            GL_UNSIGNED_BYTE, //type
            GL_TRUE, //normalized
            sizeof(Vertex), //stride
            (GLbyte *)0 + 4*3 //offset
        );
        glEnableVertexAttribArray(color_texture_program.Color_vec4);

        glVertexAttribPointer(
            color_texture_program.TexCoord_vec2, //attribute
            2, //size
            GL_FLOAT, //type
            GL_FALSE, //normalized
            sizeof(Vertex), //stride
            (GLbyte *)0 + 4*3 + 4*1 //offset
        );
        glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

        //done referring to vertex_buffer, so unbind it:
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //done setting up vertex array object, so unbind it:
        glBindVertexArray(0);

        GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
    }
    
    { //solid white texture:
        //ask OpenGL to fill white_tex with the name of an unused texture object:
        glGenTextures(1, &white_tex);

        //bind that texture object as a GL_TEXTURE_2D-type texture:
        glBindTexture(GL_TEXTURE_2D, white_tex);

        //upload a 1x1 image of solid white to the texture:
        glm::uvec2 size = glm::uvec2(1,1);
        std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

        //set filtering and wrapping parameters:
        //(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        //since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
        glGenerateMipmap(GL_TEXTURE_2D);

        //Okay, texture uploaded, can unbind it:
        glBindTexture(GL_TEXTURE_2D, 0);

        GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
    }
    
}

ZeusMode::~ZeusMode() {
    //----- free OpenGL resources -----
    glDeleteBuffers(1, &vertex_buffer);
    vertex_buffer = 0;

    glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
    vertex_buffer_for_color_texture_program = 0;

    glDeleteTextures(1, &white_tex);
    white_tex = 0;
}

bool ZeusMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    if (evt.type == SDL_MOUSEMOTION) {
        //convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
        glm::vec2 clip_mouse = glm::vec2(
            (evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
            (evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
        );
        cloud.x = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).x;
        
        //if not fire, bullet move with cloud
        if(!bullet_fired){
            bullet.x = cloud.x;
        }
    }
    
    if(evt.type == SDL_MOUSEBUTTONUP){
        //fire bullet
        bullet_fired = true;
    }

    return false;
}

void ZeusMode::update(float elapsed){
    
    static std::mt19937 mt;     //mersenne twister pseudo-random number generator
    
    { //building generate ai
        ai_offset_update -= elapsed;        //update and timing
        if (ai_offset_update < spawn_cd_min && buildings.size() < max_buildings) {
            //update again in [min,max) seconds:
            ai_offset_update = (mt() / float(mt.max())) * (spawn_cd_max + spawn_cd_min) + spawn_cd_min; //choose a random time to grow
            ai_offset = (mt() / float(mt.max()) * 2.0f - 1.0f) * (scene_radius.x + buildings_width) * 0.5f;
        
            //bound buildings in the scene
            buildings.push_back(glm::vec2(ai_offset, -scene_radius.y + grow_rate + 0.1f));
            buildings_radius.push_back(glm::vec2(buildings_width, grow_rate));
        }
        
        //ensure building number match
        assert(buildings.size() == buildings_radius.size());
    }
    
    //clamp cloud to scene:
    cloud.x = std::max(cloud.x, -scene_radius.x + cloud_radius.x);
    cloud.x = std::min(cloud.x,  scene_radius.x - cloud_radius.x);
    
    
    //----- bullet update -----
    if(bullet_fired){
        //gravitational acceleration:
        bullet += elapsed * bullet_velocity + 0.5f * gravity * elapsed * elapsed;     //displacement formula
        bullet_velocity += gravity * elapsed;                                         //update velocity
    }else{
        //when not fired, move with cloud
        bullet = cloud;
        bullet_velocity = glm::vec2(0.0f, 0.0f);
    }
    
    
    //---- collision handling ----
    
    //buildings:
    for(size_t i = 0; i < buildings.size(); i++){
        //compute area of overlap:
        glm::vec2 min = glm::max(buildings[i] - buildings_radius[i], bullet - bullet_radius);
        glm::vec2 max = glm::min(buildings[i] + buildings_radius[i], bullet + bullet_radius);
        
        //if no overlap, no collision:
        if (min.x > max.x || min.y > max.y) continue;
        
        //TODO: add random x velocity when collision
        if (max.x - min.x > max.y - min.y) {
            //wider overlap in x => bounce in y direction:
            if (bullet.y > buildings[i].y) {
                bullet.y = buildings[i].y + buildings_radius[i].y + bullet_radius.y;
                bullet_velocity.y = std::abs(bullet_velocity.y) * 0.5f;         //velocity drop in half
            } else {
                bullet.y = buildings[i].y - buildings_radius[i].y - bullet_radius.y;
                bullet_velocity.y = -std::abs(bullet_velocity.y) * 0.5f;        //velocity drop in half
            }
            
            //erase building
            buildings.erase(buildings.begin() + i);
            buildings_radius.erase(buildings_radius.begin() + i);
            i--;
        }else{
            //wider overlap in y => bounce in x direction:
            if (bullet.x > buildings[i].x) {
                bullet.x = buildings[i].x + buildings_radius[i].x + bullet_radius.x;
                bullet_velocity.x = std::abs(bullet_velocity.x) * 0.5f;
            } else {
                bullet.x = buildings[i].x - buildings_radius[i].x - bullet_radius.x;
                bullet_velocity.x = -std::abs(bullet_velocity.x) * 0.5f;
            }
            //TODO: do i need this?
            //warp y velocity based on offset from paddle center:
            float vel = (bullet.y - buildings[i].y) / (buildings_radius[i].y + bullet_radius.y);
            bullet_velocity.y = glm::mix(bullet_velocity.y, vel, 0.75f);
            
            //erase building
            buildings.erase(buildings.begin() + i);
            buildings_radius.erase(buildings_radius.begin() + i);
            i--;
        }
        
    }
    
    
    //scene walls:
    if(bullet_fired){
        //TODO: may abandon this, never hit top wall
        if (bullet.y > scene_radius.y - bullet_radius.y) {
            bullet.y = scene_radius.y - bullet_radius.y;
            if (bullet_velocity.y > 0.0f) {
                bullet_velocity.y = -bullet_velocity.y;
            }
        }
        if (bullet.y < -scene_radius.y + bullet_radius.y) {
            bullet.y = -scene_radius.y + bullet_radius.y;
            //bullet touch lower wall
            if (bullet_velocity.y < 0.0f) {
                //bullet reset state
                bullet_velocity = glm::vec2(0.0f, 0.0f);
                bullet = cloud;             //send back to cloud center
                bullet_fired = false;       //reset fire state
            }
        }

        if (bullet.x > scene_radius.x - bullet_radius.x) {
            bullet.x = scene_radius.x - bullet_radius.x;
            if (bullet_velocity.x > 0.0f) {
                bullet_velocity.x = -bullet_velocity.x;
            }
        }
        if (bullet.x < -scene_radius.x + bullet_radius.x) {
            bullet.x = -scene_radius.x + bullet_radius.x;
            if (bullet_velocity.x < 0.0f) {
                bullet_velocity.x = -bullet_velocity.x;
            }
        }
    }
    
    //----- building growth -----
    if(grow_update > grow_cd){
        grow_update = 0.0f;
        for(size_t i = 0; i < buildings.size(); i++){
            buildings_radius[i].y += grow_rate;
            buildings[i].y += grow_rate;
            
            assert( buildings[i].y - buildings_radius[i].y >= -scene_radius.y);    //ensure attach lower wall
        }
    }else{
        grow_update += elapsed;
    }
    
    
    //----- gradient trails -----
    
    //age up all locations in bullet trail:
    for (auto &t : bullet_trail) {
        t.z += elapsed;
    }
    //store fresh location at back of ball trail:
    bullet_trail.emplace_back(bullet, 0.0f);

    //trim any too-old locations from back of trail:
    //NOTE: since trail drawing interpolates between points, only removes back element if second-to-back element is too old:
    while (bullet_trail.size() >= 2 && bullet_trail[1].z > trail_length) {
        bullet_trail.pop_front();
    }
    
}

void ZeusMode::draw(glm::uvec2 const &drawable_size){
    //TODO: need to select color for each game object
    
    //some nice colors from the course web page:
    #define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
    const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x193b59ff);
    const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xf2d2b6ff);
    const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xf2ad94ff);
    const glm::u8vec4 bullet_color = HEX_TO_U8VEC4(0x00BE6100);
    const glm::u8vec4 building_color = HEX_TO_U8VEC4(0x00707070);
    const std::vector< glm::u8vec4 > trail_colors = {
        HEX_TO_U8VEC4(0xf2ad9488),
        HEX_TO_U8VEC4(0xf2897288),
        HEX_TO_U8VEC4(0xbacac088),
    };
    #undef HEX_TO_U8VEC4

    //other useful drawing constants:
    const float wall_radius = 0.05f;
    const float shadow_offset = 0.07f;
    const float padding = 0.14f; //padding between outside of walls and edge of window

    //---- compute vertices to draw ----

    //vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
    std::vector< Vertex > vertices;

    //inline helper function for rectangle drawing:
    auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
        //draw rectangle as two CCW-oriented triangles:
        vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
        vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
        vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

        vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
        vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
        vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
    };
    
    //add shadows for everything (except the trail):

    glm::vec2 s = glm::vec2(0.0f,-shadow_offset);

    draw_rectangle(glm::vec2(-scene_radius.x-wall_radius, 0.0f)+s, glm::vec2(wall_radius, scene_radius.y + 2.0f * wall_radius), shadow_color);
    draw_rectangle(glm::vec2( scene_radius.x+wall_radius, 0.0f)+s, glm::vec2(wall_radius, scene_radius.y + 2.0f * wall_radius), shadow_color);
    draw_rectangle(glm::vec2( 0.0f,-scene_radius.y-wall_radius)+s, glm::vec2(scene_radius.x, wall_radius), shadow_color);
    draw_rectangle(glm::vec2( 0.0f, scene_radius.y+wall_radius)+s, glm::vec2(scene_radius.x, wall_radius), shadow_color);
    draw_rectangle(cloud+s, cloud_radius, shadow_color);                        //shadow for cloud
    draw_rectangle(bullet+s, bullet_radius, shadow_color);                      //shadow for bullet
    
    for(size_t i = 0; i < buildings.size(); i++){
        draw_rectangle(buildings[i], buildings_radius[i], shadow_color);   //shadow for building
    }
    
    //ball's trail:
    if (bullet_trail.size() >= 2) {
        //start ti at second element so there is always something before it to interpolate from:
        std::deque< glm::vec3 >::iterator ti = bullet_trail.begin() + 1;
        //draw trail from oldest-to-newest:
        constexpr uint32_t STEPS = 20;
        //draw from [STEPS, ..., 1]:
        for (uint32_t step = STEPS; step > 0; --step) {
            //time at which to draw the trail element:
            float t = step / float(STEPS) * trail_length;
            //advance ti until 'just before' t:
            while (ti != bullet_trail.end() && ti->z > t) ++ti;
            //if we ran out of recorded tail, stop drawing:
            if (ti == bullet_trail.end()) break;
            //interpolate between previous and current trail point to the correct time:
            glm::vec3 a = *(ti-1);
            glm::vec3 b = *(ti);
            glm::vec2 at = (t - a.z) / (b.z - a.z) * (glm::vec2(b) - glm::vec2(a)) + glm::vec2(a);

            //look up color using linear interpolation:
            //compute (continuous) index:
            float c = (step-1) / float(STEPS-1) * trail_colors.size();
            //split into an integer and fractional portion:
            int32_t ci = int32_t(std::floor(c));
            float cf = c - ci;
            //clamp to allowable range (shouldn't ever be needed but good to think about for general interpolation):
            if (ci < 0) {
                ci = 0;
                cf = 0.0f;
            }
            if (ci > int32_t(trail_colors.size())-2) {
                ci = int32_t(trail_colors.size())-2;
                cf = 1.0f;
            }
            //do the interpolation (casting to floating point vectors because glm::mix doesn't have an overload for u8 vectors):
            glm::u8vec4 color = glm::u8vec4(
                glm::mix(glm::vec4(trail_colors[ci]), glm::vec4(trail_colors[ci+1]), cf)
            );

            //draw:
            draw_rectangle(at, bullet_radius, color);
        }
    }
    
    
    //solid objects:

    //walls:
    draw_rectangle(glm::vec2(-scene_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, scene_radius.y + 2.0f * wall_radius), fg_color);
    draw_rectangle(glm::vec2( scene_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, scene_radius.y + 2.0f * wall_radius), fg_color);
    draw_rectangle(glm::vec2( 0.0f,-scene_radius.y-wall_radius), glm::vec2(scene_radius.x, wall_radius), fg_color);
    draw_rectangle(glm::vec2( 0.0f, scene_radius.y+wall_radius), glm::vec2(scene_radius.x, wall_radius), fg_color);
    
    //cloud:
    draw_rectangle(cloud, cloud_radius, fg_color);      //TODO: need to change this color
    
    //bullet:
    draw_rectangle(bullet, bullet_radius, bullet_color);    //TODO: need to change this color
    
    //buildings:
    for(size_t i = 0; i < buildings.size(); i++){
        draw_rectangle(buildings[i], buildings_radius[i], building_color);
    }
    
    //TODO: do i need scores?
    //scores:
    glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
    for (uint32_t i = 0; i < score; ++i) {
        draw_rectangle(glm::vec2( scene_radius.x - (2.0f + 3.0f * i) * score_radius.x, scene_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
    }
    
    
    //------ compute court-to-window transform ------

    //compute area that should be visible:
    glm::vec2 scene_min = glm::vec2(
        -scene_radius.x - 2.0f * wall_radius - padding,
        -scene_radius.y - 2.0f * wall_radius - padding
    );
    glm::vec2 scene_max = glm::vec2(
        scene_radius.x + 2.0f * wall_radius + padding,
        scene_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
    );

    //compute window aspect ratio:
    float aspect = drawable_size.x / float(drawable_size.y);
    //we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

    //compute scale factor for court given that...
    float scale = std::min(
        (2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
        (2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
    );

    glm::vec2 center = 0.5f * (scene_max + scene_min);

    //build matrix that scales and translates appropriately:
    glm::mat4 court_to_clip = glm::mat4(
        glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, scale, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
    );
    //NOTE: glm matrices are specified in *Column-Major* order,
    // so each line above is specifying a *column* of the matrix(!)

    //also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
    clip_to_court = glm::mat3x2(
        glm::vec2(aspect / scale, 0.0f),
        glm::vec2(0.0f, 1.0f / scale),
        glm::vec2(center.x, center.y)
    );
    
    
    //---- actual drawing ----

    //clear the color buffer:
    glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //use alpha blending:
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //don't use the depth test:
    glDisable(GL_DEPTH_TEST);

    //upload vertices to vertex_buffer:
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //set color_texture_program as current program:
    glUseProgram(color_texture_program.program);

    //upload OBJECT_TO_CLIP to the proper uniform location:
    glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

    //use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
    glBindVertexArray(vertex_buffer_for_color_texture_program);

    //bind the solid white texture to location zero so things will be drawn just with their colors:
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, white_tex);

    //run the OpenGL pipeline:
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

    //unbind the solid white texture:
    glBindTexture(GL_TEXTURE_2D, 0);

    //reset vertex array to none:
    glBindVertexArray(0);

    //reset current program to none:
    glUseProgram(0);
    

    GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.
    
};
