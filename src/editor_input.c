#include <math.h>

#include "app_internal.h"
#include "atom_forge/chemistry.h"

void editor_refresh_dimensions(app_t* app) {
  SDL_GetWindowSize(app->window, &app->window_width, &app->window_height);
  SDL_GetWindowSizeInPixels(app->window, &app->width, &app->height);
  if (app->width < 1) {
    app->width = 1;
  }
  if (app->height < 1) {
    app->height = 1;
  }
  if (app->window_width < 1) {
    app->window_width = 1;
  }
  if (app->window_height < 1) {
    app->window_height = 1;
  }
}

/**
 * @brief Converts an SDL logical mouse position into renderer pixel space.
 *
 * @param app Application state.
 * @param x Mouse X coordinate reported by SDL.
 * @param y Mouse Y coordinate reported by SDL.
 *
 * @return Nothing.
 *
 * @note Updates app->mouse_x and app->mouse_y. Also refreshes the current
 *       window dimensions.
 * @note Performs scaling only; it does not convert to world coordinates.
 */
static void editor_set_mouse_position(app_t* app, float x, float y) {
  editor_refresh_dimensions(app);
  app->mouse_x = x * (float)app->width / (float)app->window_width;
  app->mouse_y = y * (float)app->height / (float)app->window_height;
}

/**
 * @brief Converts a screen-space position into world-space coordinates.
 *
 * @param app Application state containing camera, zoom, and dimensions.
 * @param screen_x Horizontal screen coordinate in renderer pixels.
 * @param screen_y Vertical screen coordinate in renderer pixels.
 *
 * @return A vec2_t containing the corresponding world position.
 *
 * @note The vertical axis is inverted because screen Y increases
 *       downward while world Y increases upward.
 * @note The caller must ensure app->zoom is non-zero.
 */
static vec2_t editor_screen_to_world(const app_t* app, float screen_x, float screen_y) {
  return vec2(
    app->camera.x + (screen_x - (float)app->width * 0.5f) / app->zoom,
    app->camera.y - (screen_y - (float)app->height * 0.5f) / app->zoom
  );
}

/**
 * @brief Tests whether the pointer lies in a reserved HUD region.
 *
 * @param app Application state containing the current mouse Y coordinate
 *            and viewport height.
 *
 * @return true if the pointer is above the top threshold or below
 *         the bottom threshold; otherwise, false.
 *
 * @note The reserved regions are defined as the first 59 pixels
 *       and the last 70 pixels of renderer space.
 */
static bool editor_pointer_over_ui(const app_t* app) {
  return app->mouse_y < 59.0f || app->mouse_y > (float)app->height - 70.0f;
}

/**
 * @brief Maps an editor tool to the particle kind to place.
 *
 * @param tool Active editor tool.
 *
 * @return particle_neutron for the neutron tool,
 *         particle_electron for the electron tool,
 *         particle_proton for the proton tool or any other value.
 *
 * @note The select tool has no particle-placement behavior. Its fallback
 *       return value is only used if the helper is called with that tool.
 */
static particle_kind_t editor_tool_particle(editor_tool_t tool) {
  switch (tool) {
    case editor_tool_neutron: return particle_neutron;
    case editor_tool_electron: return particle_electron;
    case editor_tool_proton:
    case editor_tool_select:
    default: return particle_proton;
  }
}

/**
 * @brief Processes a single SDL keyboard-down event.
 *
 * @param app Application state that may be modified.
 * @param key_event SDL keyboard event to process.
 *
 * @return Nothing.
 *
 * @note May change the active tool, pause state, selection, world contents,
 *       or workspace save file, and may set a status notice.
 * @note Keyboard-repeat events are ignored.
 */
static void editor_handle_key(app_t* app, const SDL_KeyboardEvent* key_event) {
  if (key_event->repeat) {
    return;
  }
  SDL_Keycode key = key_event->key;
  bool control = (key_event->mod & SDL_KMOD_CTRL) != 0;

  if (control && key == SDLK_S) {
    if (world_save(&app->world, "atom_forge.save")) {
      editor_set_notice(app, "SAVED atom_forge.save");
    } else {
      editor_set_notice(app, "SAVE FAILED");
    }
    return;
  }
  if (control && key == SDLK_L) {
    if (world_load(&app->world, "atom_forge.save")) {
      chemistry_rebuild(&app->world);
      editor_set_notice(app, "LOADED atom_forge.save");
    } else {
      editor_set_notice(app, "LOAD FAILED - NO VALID SAVE");
    }
    return;
  }

  switch (key) {
    case SDLK_0:
      app->world.tool = editor_tool_select;
      editor_set_notice(app, "SELECT TOOL: CLICK AND DRAG PARTICLES");
      break;
    case SDLK_1:
      app->world.tool = editor_tool_proton;
      editor_set_notice(app, "PROTON TOOL: LEFT CLICK TO PLACE");
      break;
    case SDLK_2:
      app->world.tool = editor_tool_neutron;
      editor_set_notice(app, "NEUTRON TOOL: LEFT CLICK TO PLACE");
      break;
    case SDLK_3:
      app->world.tool = editor_tool_electron;
      editor_set_notice(app, "ELECTRON TOOL: LEFT CLICK TO PLACE");
      break;
    case SDLK_SPACE:
      app->space_down = true;
      app->world.paused = !app->world.paused;
      editor_set_notice(app, app->world.paused ? "SIMULATION PAUSED" : "SIMULATION RUNNING");
      break;
    case SDLK_DELETE:
    case SDLK_BACKSPACE:
      if (world_remove_particle(&app->world, app->world.selected_particle)) {
        chemistry_rebuild(&app->world);
        editor_set_notice(app, "PARTICLE DELETED");
      }
      break;
    case SDLK_R:
      world_clear(&app->world);
      chemistry_rebuild(&app->world);
      editor_set_notice(app, "WORKSPACE CLEARED");
      break;
    case SDLK_ESCAPE:
      app->world.tool = editor_tool_select;
      app->world.selected_particle = -1;
      break;
    default:
      break;
  }
}

/**
 * @brief Processes a single mouse-button-down event.
 *
 * @param app Application state that may be modified.
 * @param button_event SDL mouse-button event.
 *
 * @return Nothing.
 *
 * @note May start a pan or particle drag, select or delete a particle, place
 *       a particle, or rebuild chemistry.
 * @note Placement is snapped to integer world coordinates.
 * @note Particle editing is blocked when the pointer lies over the HUD
 *       or when Space is held.
 */
static void editor_handle_mouse_down(app_t* app, const SDL_MouseButtonEvent* button_event) {
  editor_set_mouse_position(app, button_event->x, button_event->y);
  app->last_mouse_x = app->mouse_x;
  app->last_mouse_y = app->mouse_y;

  if (button_event->button == SDL_BUTTON_MIDDLE) {
    app->middle_down = true;
    return;
  }
  if (button_event->button == SDL_BUTTON_RIGHT) {
    if (!editor_pointer_over_ui(app)) {
      vec2_t world_position = editor_screen_to_world(app, app->mouse_x, app->mouse_y);
      float radius = fmaxf(0.34f, 13.0f / app->zoom);
      int slot = world_find_particle_at(&app->world, world_position, radius);
      if (slot >= 0) {
        (void)world_remove_particle(&app->world, slot);
        chemistry_rebuild(&app->world);
      }
    }
    return;
  }
  if (button_event->button != SDL_BUTTON_LEFT) {
    return;
  }

  app->left_down = true;
  if (app->space_down) {
    return;
  }
  if (editor_pointer_over_ui(app)) {
    return;
  }

  vec2_t world_position = editor_screen_to_world(app, app->mouse_x, app->mouse_y);
  if (app->world.tool == editor_tool_select) {
    float radius = fmaxf(0.40f, 14.0f / app->zoom);
    int slot = world_find_particle_at(&app->world, world_position, radius);
    app->world.selected_particle = slot;
    app->dragged_particle = slot;
    app->dragging_particle = slot >= 0;
  } else {
    world_position.x = roundf(world_position.x);
    world_position.y = roundf(world_position.y);
    int slot = world_add_particle(
      &app->world,
      editor_tool_particle(app->world.tool),
      world_position
    );
    if (slot < 0) {
      editor_set_notice(app, "PARTICLE LIMIT REACHED (256)");
    } else {
      chemistry_rebuild(&app->world);
    }
  }
}

/* TODO description */
static void editor_handle_mouse_up(app_t* app, const SDL_MouseButtonEvent* button_event) {
  if (button_event->button == SDL_BUTTON_LEFT) {
    app->left_down = false;
    app->dragging_particle = false;
    app->dragged_particle = -1;
  } else if (button_event->button == SDL_BUTTON_MIDDLE) {
    app->middle_down = false;
  }
}

/* TODO description */
static void editor_handle_mouse_motion(app_t* app, const SDL_MouseMotionEvent* motion_event) {
  editor_set_mouse_position(app, motion_event->x, motion_event->y);
  float dx = app->mouse_x - app->last_mouse_x;
  float dy = app->mouse_y - app->last_mouse_y;

  if (app->middle_down || (app->space_down && app->left_down)) {
    app->camera.x -= dx / app->zoom;
    app->camera.y += dy / app->zoom;
  } else if (app->dragging_particle && app->left_down) {
    particle_t* particle = world_get_particle(&app->world, app->dragged_particle);
    if (particle != NULL) {
      particle->position = editor_screen_to_world(app, app->mouse_x, app->mouse_y);
      particle->velocity = vec2(0.0f, 0.0f);
      particle->decay_time = 0.0f;
      chemistry_rebuild(&app->world);
    }
  }

  app->last_mouse_x = app->mouse_x;
  app->last_mouse_y = app->mouse_y;
}

/* TODO description */
static void editor_handle_wheel(app_t* app, const SDL_MouseWheelEvent* wheel_event) {
  float mouse_x = 0.0f;
  float mouse_y = 0.0f;
  (void)SDL_GetMouseState(&mouse_x, &mouse_y);
  editor_set_mouse_position(app, mouse_x, mouse_y);
  vec2_t before = editor_screen_to_world(app, app->mouse_x, app->mouse_y);
  float factor = powf(1.12f, wheel_event->y);
  app->zoom = fmaxf(12.0f, fminf(130.0f, app->zoom * factor));
  vec2_t after = editor_screen_to_world(app, app->mouse_x, app->mouse_y);
  app->camera = vec2_add(app->camera, vec2_sub(before, after));
}

void editor_handle_events(app_t* app) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_QUIT:
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        app->running = false;
        break;
      case SDL_EVENT_KEY_DOWN:
        editor_handle_key(app, &event.key);
        break;
      case SDL_EVENT_KEY_UP:
        if (event.key.key == SDLK_SPACE) {
          app->space_down = false;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        editor_handle_mouse_down(app, &event.button);
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        editor_handle_mouse_up(app, &event.button);
        break;
      case SDL_EVENT_MOUSE_MOTION:
        editor_handle_mouse_motion(app, &event.motion);
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        editor_handle_wheel(app, &event.wheel);
        break;
      default:
        break;
    }
  }
}
