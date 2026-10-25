#ifndef ATOM_FORGE_RENDERER_INTERNAL_H
  #define ATOM_FORGE_RENDERER_INTERNAL_H

  #include <glad/gl.h>
  #include <stdbool.h>
  #include <stddef.h>

  #include "atom_forge/renderer.h"

  #define renderer_max_overlay_vertices 30000
  #define renderer_max_particle_instances (max_particles * 3)

  struct renderer {
    bool initialized;
    GLuint grid_program;
    GLuint particle_program;
    GLuint overlay_program;
    GLuint grid_vao;
    GLuint particle_vao;
    GLuint overlay_vao;
    GLuint particle_quad_vbo;
    GLuint particle_instance_vbo;
    GLuint overlay_vbo;
    GLint grid_resolution_location;
    GLint grid_camera_location;
    GLint grid_zoom_location;
    GLint grid_time_location;
    GLint particle_resolution_location;
    GLint particle_camera_location;
    GLint particle_zoom_location;
    GLint particle_time_location;
    GLint overlay_resolution_location;
    float width;
    float height;
    vec2_t camera;
    float zoom;
    float time;
    float* overlay_vertices;
    size_t overlay_vertex_count;
    size_t overlay_vertex_capacity;
  };

#endif
