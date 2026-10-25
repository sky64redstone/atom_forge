/*
 * Private header: src/app_internal.h
 * Implementation: src/editor_ui.c & src/editor_input.c
 *
 * This module translates keyboard and mouse events into changes to the world
 * and camera. It is the boundary between SDL's input representation and the
 * application's world-coordinate model.
 */

#ifndef ATOM_FORGE_APP_INTERNAL_H
  #define ATOM_FORGE_APP_INTERNAL_H

  #include <SDL3/SDL.h>
  #include <stdbool.h>

  #include "atom_forge/renderer.h"
  #include "atom_forge/vec2.h"
  #include "atom_forge/world.h"

  /* State shared only by the application and editor functions. */
  typedef struct app {
    SDL_Window* window;
    SDL_GLContext gl_context;
    renderer_t* renderer;
    world_t world;
    bool running;
    bool left_down;
    bool middle_down;
    bool space_down;
    bool dragging_particle;
    int dragged_particle;
    vec2_t camera;
    float zoom;
    float mouse_x;
    float mouse_y;
    float last_mouse_x;
    float last_mouse_y;
    int width;
    int height;
    int window_width;
    int window_height;
    float notice_time;
    char notice[96];
    float title_update_time;
    float app_time;
  } app_t;

  /**
   * @brief Sets a message to display temporarily in the editor.
   *
   * @param app Application state containing the notice buffer.
   * @param message Null-terminated message to display.
   *
   * @return Nothing.
   *
   * @note Copies the message into app->notice and resets the display timer
   *       to 2.4 seconds.
   */
  void editor_set_notice(app_t* app, const char* message);

  /**
   * @brief Refreshes logical window dimensions and physical pixel dimensions.
   *
   * @param app Application state whose window is queried.
   *
   * @return Nothing.
   *
   * @note width and height are used for rendering; the window dimensions
   *       are used to convert SDL mouse coordinates into pixel coordinates.
   */
  void editor_refresh_dimensions(app_t* app);

  /**
   * @brief Polls SDL events and dispatches them to the editor handlers.
   *
   * @param app Application state to update.
   *
   * @return Nothing.
   *
   * @note May change app->running, world state, editor state,
   *       camera position, or zoom.
   * @note Drains the currently available SDL event queue.
   */
  void editor_handle_events(app_t* app);

  /**
   * @brief Updates time-dependent editor UI state.
   *
   * @param app Application state to update.
   * @param delta_time Elapsed time in seconds.
   *
   * @return Nothing.
   *
   * @note Decrements the temporary notice timer and updates the
   *       window title when due.
   * @note Does not advance physics.
   */
  void editor_update(app_t* app, float delta_time);

  /**
   * @brief Draws the editor's labels and HUD.
   *
   * @param app Application state containing world and renderer.
   *
   * @return Nothing.
   *
   * @note Queues atom labels and HUD geometry for the current frame.
   * @note Returns early if app or app->renderer is NULL.
   */
  void editor_draw_ui(const app_t* app);

#endif
