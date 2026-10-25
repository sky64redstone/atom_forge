#include <glad/gl.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom_forge/renderer.h"

#include "renderer_internal.h"
#include "renderer_shaders.h"

typedef struct particle_instance {
  float center_x;
  float center_y;
  float radius;
  float red;
  float green;
  float blue;
  float alpha;
  float mode;
} particle_instance_t;

static GLuint renderer_compile_shader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);
  if (shader == 0) {
    return 0;
  }
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint compiled = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE) {
    char log[2048] = { 0 };
    GLsizei log_length = 0;
    glGetShaderInfoLog(shader, (GLsizei)sizeof(log), &log_length, log);
    (void)fprintf(stderr, "shader compile failed: %s\n", log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static GLuint renderer_create_program(
  const char* vertex_source,
  const char* fragment_source
) {
  GLuint vertex_shader = renderer_compile_shader(GL_VERTEX_SHADER, vertex_source);
  GLuint fragment_shader = renderer_compile_shader(GL_FRAGMENT_SHADER, fragment_source);
  if (vertex_shader == 0 || fragment_shader == 0) {
    if (vertex_shader != 0) {
      glDeleteShader(vertex_shader);
    }
    if (fragment_shader != 0) {
      glDeleteShader(fragment_shader);
    }
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program == 0) {
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return 0;
  }
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  GLint linked = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (linked != GL_TRUE) {
    char log[2048] = { 0 };
    GLsizei log_length = 0;
    glGetProgramInfoLog(program, (GLsizei)sizeof(log), &log_length, log);
    (void)fprintf(stderr, "shader link failed: %s\n", log);
    glDeleteProgram(program);
    program = 0;
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return program;
}

static GLint renderer_uniform(GLuint program, const char* name) {
  return glGetUniformLocation(program, name);
}

static bool renderer_initialize(renderer_t* renderer) {
  renderer->grid_program = renderer_create_program(
    renderer_grid_vertex_shader,
    renderer_grid_fragment_shader
  );
  renderer->particle_program = renderer_create_program(
    renderer_particle_vertex_shader,
    renderer_particle_fragment_shader
  );
  renderer->overlay_program = renderer_create_program(
    renderer_overlay_vertex_shader,
    renderer_overlay_fragment_shader
  );
  if (renderer->grid_program == 0 || renderer->particle_program == 0 ||
      renderer->overlay_program == 0) {
    return false;
  }

  renderer->grid_resolution_location = renderer_uniform(renderer->grid_program, "u_resolution");
  renderer->grid_camera_location = renderer_uniform(renderer->grid_program, "u_camera");
  renderer->grid_zoom_location = renderer_uniform(renderer->grid_program, "u_zoom");
  renderer->grid_time_location = renderer_uniform(renderer->grid_program, "u_time");
  renderer->particle_resolution_location = renderer_uniform(
    renderer->particle_program,
    "u_resolution"
  );
  renderer->particle_camera_location = renderer_uniform(renderer->particle_program, "u_camera");
  renderer->particle_zoom_location = renderer_uniform(renderer->particle_program, "u_zoom");
  renderer->particle_time_location = renderer_uniform(renderer->particle_program, "u_time");
  renderer->overlay_resolution_location = renderer_uniform(
    renderer->overlay_program,
    "u_resolution"
  );

  glGenVertexArrays(1, &renderer->grid_vao);
  glGenVertexArrays(1, &renderer->particle_vao);
  glGenBuffers(1, &renderer->particle_quad_vbo);
  glGenBuffers(1, &renderer->particle_instance_vbo);
  glGenVertexArrays(1, &renderer->overlay_vao);
  glGenBuffers(1, &renderer->overlay_vbo);

  const float quad_vertices[12] = {
    -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f
  };
  glBindVertexArray(renderer->particle_vao);
  glBindBuffer(GL_ARRAY_BUFFER, renderer->particle_quad_vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizeiptr)sizeof(quad_vertices),
    quad_vertices,
    GL_STATIC_DRAW
  );
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
    (GLsizei)(2 * sizeof(float)), (const void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, renderer->particle_instance_vbo);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
    (GLsizei)sizeof(particle_instance_t), (const void*)offsetof(particle_instance_t, center_x));
  glVertexAttribDivisor(1, 1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE,
    (GLsizei)sizeof(particle_instance_t), (const void*)offsetof(particle_instance_t, radius));
  glVertexAttribDivisor(2, 1);
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
    (GLsizei)sizeof(particle_instance_t), (const void*)offsetof(particle_instance_t, red));
  glVertexAttribDivisor(3, 1);
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
    (GLsizei)sizeof(particle_instance_t), (const void*)offsetof(particle_instance_t, mode));
  glVertexAttribDivisor(4, 1);

  glBindVertexArray(renderer->overlay_vao);
  glBindBuffer(GL_ARRAY_BUFFER, renderer->overlay_vbo);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
    (GLsizei)(6 * sizeof(float)), (const void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
    (GLsizei)(6 * sizeof(float)), (const void*)(2 * sizeof(float)));

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  renderer->initialized = true;
  return true;
}


void renderer_destroy(renderer_t* renderer) {
  if (renderer == NULL) {
    return;
  }
  if (renderer->grid_program != 0) {
    glDeleteProgram(renderer->grid_program);
  }
  if (renderer->particle_program != 0) {
    glDeleteProgram(renderer->particle_program);
  }
  if (renderer->overlay_program != 0) {
    glDeleteProgram(renderer->overlay_program);
  }
  if (renderer->grid_vao != 0) {
    glDeleteVertexArrays(1, &renderer->grid_vao);
  }
  if (renderer->particle_vao != 0) {
    glDeleteVertexArrays(1, &renderer->particle_vao);
  }
  if (renderer->overlay_vao != 0) {
    glDeleteVertexArrays(1, &renderer->overlay_vao);
  }
  if (renderer->particle_quad_vbo != 0) {
    glDeleteBuffers(1, &renderer->particle_quad_vbo);
  }
  if (renderer->particle_instance_vbo != 0) {
    glDeleteBuffers(1, &renderer->particle_instance_vbo);
  }
  if (renderer->overlay_vbo != 0) {
    glDeleteBuffers(1, &renderer->overlay_vbo);
  }
  free(renderer->overlay_vertices);
  free(renderer);
}

renderer_t* renderer_create(void) {
  renderer_t* renderer = calloc(1, sizeof(*renderer));
  if (renderer == NULL) {
    return NULL;
  }
  renderer->overlay_vertex_capacity = renderer_max_overlay_vertices;
  renderer->overlay_vertices = calloc(
    renderer->overlay_vertex_capacity * 6,
    sizeof(float)
  );
  if (renderer->overlay_vertices == NULL || !renderer_initialize(renderer)) {
    renderer_destroy(renderer);
    return NULL;
  }
  return renderer;
}

vec2_t renderer_world_to_screen(const renderer_t* renderer, vec2_t world_position) {
  return vec2(
    (world_position.x - renderer->camera.x) * renderer->zoom + renderer->width * 0.5f,
    renderer->height * 0.5f - (world_position.y - renderer->camera.y) * renderer->zoom
  );
}

void renderer_begin_frame(
  renderer_t* renderer,
  int width,
  int height,
  vec2_t camera,
  float zoom,
  float time
) {
  if (renderer == NULL || !renderer->initialized) {
    return;
  }
  renderer->width = (float)width;
  renderer->height = (float)height;
  renderer->camera = camera;
  renderer->zoom = zoom;
  renderer->time = time;
  renderer->overlay_vertex_count = 0;

  glViewport(0, 0, width, height);
  glClearColor(0.025f, 0.034f, 0.050f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(renderer->grid_program);
  glUniform2f(renderer->grid_resolution_location, renderer->width, renderer->height);
  glUniform2f(renderer->grid_camera_location, camera.x, camera.y);
  glUniform1f(renderer->grid_zoom_location, zoom);
  glUniform1f(renderer->grid_time_location, time);
  glBindVertexArray(renderer->grid_vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
}


void renderer_draw_world_bonds(renderer_t* renderer, const world_t* world) {
  if (renderer == NULL || world == NULL) {
    return;
  }
  const color4_t bond_color = { 0.16f, 0.55f, 0.57f, 0.68f };
  for (size_t i = 0; i < world->bond_count; ++i) {
    const chemical_bond_t* bond = &world->bonds[i];
    if (bond->atom_a >= world->atom_count || bond->atom_b >= world->atom_count) {
      continue;
    }
    vec2_t a = renderer_world_to_screen(renderer, world->atoms[bond->atom_a].position);
    vec2_t b = renderer_world_to_screen(renderer, world->atoms[bond->atom_b].position);
    renderer_draw_line(renderer, a.x, a.y, b.x, b.y, 2.0f, bond_color);
  }
}

void renderer_draw_particles(renderer_t* renderer, const world_t* world) {
  if (renderer == NULL || world == NULL) {
    return;
  }
  particle_instance_t instances[renderer_max_particle_instances];
  size_t count = 0;
  for (size_t i = 0; i < max_particles && count < renderer_max_particle_instances; ++i) {
    const particle_t* particle = &world->particles[i];
    if (!particle->alive) {
      continue;
    }
    color4_t color;
    float radius;
    switch (particle->kind) {
      case particle_proton:
        color = (color4_t){ 1.00f, 0.27f, 0.22f, 1.0f };
        radius = 0.22f;
        break;
      case particle_neutron:
        color = (color4_t){ 0.32f, 0.61f, 1.00f, 1.0f };
        radius = 0.22f;
        break;
      case particle_electron:
      default:
        color = (color4_t){ 0.20f, 0.86f, 0.96f, 1.0f };
        radius = 0.15f;
        break;
    }
    if (particle->flash_time > 0.0f) {
      color = (color4_t){ 1.00f, 0.90f, 0.58f, 1.0f };
    }
    instances[count++] = (particle_instance_t){
      particle->position.x, particle->position.y, radius,
      color.r, color.g, color.b, color.a, 0.0f
    };
  }

  for (size_t i = 0; i < world->atom_count && count < renderer_max_particle_instances; ++i) {
    const atom_t* atom = &world->atoms[i];
    if (atom->radioactive) {
      float nucleons = (float)(atom->proton_count + atom->neutron_count);
      float radius = 0.50f + 0.12f * sqrtf(nucleons);
      instances[count++] = (particle_instance_t){
        atom->position.x, atom->position.y, radius,
        1.00f, 0.32f, 0.10f, 0.90f, 2.0f
      };
    }
  }

  const particle_t* selected = world_get_particle_const(world, world->selected_particle);
  if (selected != NULL && count < renderer_max_particle_instances) {
    instances[count++] = (particle_instance_t){
      selected->position.x, selected->position.y, 0.38f,
      1.00f, 0.83f, 0.39f, 0.95f, 1.0f
    };
  }

  if (count == 0) {
    return;
  }
  glUseProgram(renderer->particle_program);
  glUniform2f(
    renderer->particle_resolution_location,
    renderer->width,
    renderer->height
  );
  glUniform2f(
    renderer->particle_camera_location,
    renderer->camera.x,
    renderer->camera.y
  );
  glUniform1f(renderer->particle_zoom_location, renderer->zoom);
  glUniform1f(renderer->particle_time_location, renderer->time);
  glBindVertexArray(renderer->particle_vao);
  glBindBuffer(GL_ARRAY_BUFFER, renderer->particle_instance_vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizeiptr)(count * sizeof(instances[0])),
    instances,
    GL_STREAM_DRAW
  );
  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
  glBindVertexArray(0);
}

