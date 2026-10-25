/*
 * Public header: include/atom_forge/renderer.h
 * Private implementation header: src/renderer_internal.h
 * Shader source: src/renderer_shaders.h
 *
 * The renderer owns all OpenGL resources used to draw the scene. Its public
 * type, renderer_t, is opaque: application code can hold a pointer to it, but
 * only the renderer implementation knows its internal GPU handles.
 * The functions in this section require a valid OpenGL context, with GLAD
 * loaded, whenever they issue OpenGL calls.
 */

#ifndef ATOM_FORGE_RENDERER_H
  #define ATOM_FORGE_RENDERER_H

  #include <stdbool.h>

  #include "atom_forge/vec2.h"
  #include "atom_forge/world.h"

  typedef struct color4 {
    float r;
    float g;
    float b;
    float a;
  } color4_t;

  /* Opaque type: OpenGL resource ownership stays private to the renderer module. */
  typedef struct renderer renderer_t;

  /* Call only after creating a current OpenGL context and loading GLAD. */
  renderer_t* renderer_create(void);
  void renderer_destroy(renderer_t* renderer);

  /**
   * @brief Begins a frame, updates camera/viewport state, clears the screen,
   *        and draws the procedural grid.
   *
   * @param renderer Renderer instance.
   * @param width Viewport width in pixels.
   * @param height Viewport height in pixels.
   * @param camera Camera center in world coordinates.
   * @param zoom World-to-screen scale.
   * @param time Application time used by animated shader effects.
   *
   * @return Nothing.
   *
   * @note Updates renderer frame state, clears pending overlay geometry,
   *       sets the OpenGL viewport, clears the color buffer, sets grid
   *       uniforms, and draws a full-screen triangle.
   * @note No action occurs for a NULL or uninitialized renderer.
   */
  void renderer_begin_frame(
    renderer_t* renderer,
    int width,
    int height,
    vec2_t camera,
    float zoom,
    float time
  );

  vec2_t renderer_world_to_screen(const renderer_t* renderer, vec2_t world_position);
  void renderer_draw_line(
    renderer_t* renderer,
    float x0,
    float y0,
    float x1,
    float y1,
    float thickness,
    color4_t color
  );
  void renderer_draw_rect(
    renderer_t* renderer,
    float x,
    float y,
    float width,
    float height,
    color4_t color
  );
  void renderer_draw_text(
    renderer_t* renderer,
    float x,
    float y,
    const char* text,
    int scale,
    color4_t color
  );

  /**
   * @brief Uploads and draws all queued overlay vertices.
   *
   * @param renderer Renderer whose pending overlay batch should be flushed.
   *
   * @return Nothing.
   *
   * @note Uploads buffered vertices using GL_STREAM_DRAW, binds the overlay
   *       program and VAO, issues glDrawArrays(), and resets
   *       overlay_vertex_count.
   * @note Returns if the renderer is NULL, uninitialized, or has no vertices.
   */
  void renderer_flush_overlay(renderer_t* renderer);

  /**
   * @brief Queues a line for every valid chemical bond in the world.
   *
   * @param renderer Renderer used to convert coordinates and queue lines.
   * @param world World containing atom positions and the bond array.
   *
   * @return Nothing.
   *
   * @note Appends bond-line triangles to the renderer overlay buffer.
   * @note Invalid atom indices are ignored.
   * @note This function queues geometry; it does not flush the overlay itself.
   */
  void renderer_draw_world_bonds(renderer_t* renderer, const world_t* world);

  /**
   * @brief Draws particles, radioactive rings, and the selected-particle ring
   *        using instanced GPU rendering.
   *
   * @param renderer Renderer used to issue the instanced draw call.
   * @param world World containing particle and atom data.
   *
   * @return Nothing.
   *
   * @note Builds a temporary particle-instance array, uploads it to a
   *       streaming VBO, binds the particle shader, and issues
   *       glDrawArraysInstanced().
   * @note Returns early if either pointer is NULL or there is nothing to draw.
   * @note Particle radius values are specified in world units, subject
   *       to a minimum/maximum pixel radius in the shader.
   */
  void renderer_draw_particles(renderer_t* renderer, const world_t* world);

  /**
   * @brief Flushes any remaining overlay geometry for the current frame.
   *
   * @param renderer Renderer whose pending geometry should be drawn.
   *
   * @return Nothing.
   *
   * @note Calls renderer_flush_overlay().
   * @note This function does not swap the SDL window buffers. The application
   *       performs that operation with SDL_GL_SwapWindow().
   */
  void renderer_present(renderer_t* renderer);

#endif
