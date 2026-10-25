#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <stdio.h>

#include "app_internal.h"

#include "atom_forge/chemistry.h"
#include "atom_forge/physics.h"

static void app_render(const app_t* app);
static void app_shutdown(app_t* app);

int main(void) {
  app_t app = { 0 };
  app.dragged_particle = -1;
  app.camera = vec2(0.0f, 0.0f);
  app.zoom = 42.0f;
  app.running = true;
  world_init(&app.world);

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    (void)fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  (void)SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  (void)SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  (void)SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  (void)SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  (void)SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

  app.window = SDL_CreateWindow(
    "atom forge",
    1280,
    800,
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
  );
  if (app.window == NULL) {
    (void)fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  app.gl_context = SDL_GL_CreateContext(app.window);
  if (app.gl_context == NULL) {
    (void)fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return 1;
  }
  if (!SDL_GL_MakeCurrent(app.window, app.gl_context)) {
    (void)fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
    app_shutdown(&app);
    return 1;
  }

  int gl_version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
  int gl_major = GLAD_VERSION_MAJOR(gl_version);
  int gl_minor = GLAD_VERSION_MINOR(gl_version);
  if (gl_version == 0 || gl_major < 3 || (gl_major == 3 && gl_minor < 3)) {
    (void)fprintf(stderr, "OpenGL 3.3 or newer is required.\n");
    app_shutdown(&app);
    return 1;
  }

  app.renderer = renderer_create();
  if (app.renderer == NULL) {
    (void)fprintf(stderr, "Renderer initialization failed.\n");
    app_shutdown(&app);
    return 1;
  }
  (void)SDL_GL_SetSwapInterval(1);

  editor_refresh_dimensions(&app);
  chemistry_rebuild(&app.world);
  editor_set_notice(&app, "1 PROTON  2 NEUTRON  3 ELECTRON  |  SPACE TO RUN");

  Uint64 last_counter = SDL_GetPerformanceCounter();
  Uint64 frequency = SDL_GetPerformanceFrequency();
  if (frequency == 0) {
    frequency = 1;
  }

  while (app.running) {
    /* Events */
    editor_handle_events(&app);
    editor_refresh_dimensions(&app);

    /* Updates */
    Uint64 current_counter = SDL_GetPerformanceCounter();
    float delta_time = (float)((double)(current_counter - last_counter) / (double)frequency);
    last_counter = current_counter;
    if (delta_time > 0.05f) {
      (void)fprintf(stderr, "WARNING: delta_time = %f\n", delta_time);
      delta_time = 0.05f;
    }
    app.app_time += delta_time;
    physics_update(&app.world, delta_time);
    editor_update(&app, delta_time);

    /* Rendering */
    app_render(&app);
  }

  app_shutdown(&app);
  return 0;
}

static void app_render(const app_t* app) {
  renderer_begin_frame(
    app->renderer,
    app->width,
    app->height,
    app->camera,
    app->zoom,
    app->app_time
  );
  renderer_draw_world_bonds(app->renderer, &app->world);
  renderer_flush_overlay(app->renderer);
  renderer_draw_particles(app->renderer, &app->world);
  editor_draw_ui(app);
  renderer_present(app->renderer);
  SDL_GL_SwapWindow(app->window);
}

static void app_shutdown(app_t* app) {
  renderer_destroy(app->renderer);
  app->renderer = NULL;
  if (app->gl_context != NULL) {
    SDL_GL_DestroyContext(app->gl_context);
    app->gl_context = NULL;
  }
  if (app->window != NULL) {
    SDL_DestroyWindow(app->window);
    app->window = NULL;
  }
  SDL_Quit();
}

