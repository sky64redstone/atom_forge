#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app_internal.h"
#include "atom_forge/chemistry.h"

static const color4_t color_white = { 0.88f, 0.92f, 0.96f, 1.0f };
static const color4_t color_muted = { 0.47f, 0.56f, 0.64f, 1.0f };
static const color4_t color_panel = { 0.018f, 0.025f, 0.038f, 0.90f };
static const color4_t color_panel_edge = { 0.18f, 0.27f, 0.32f, 0.85f };
static const color4_t color_warn = { 1.0f, 0.47f, 0.19f, 1.0f };
static const color4_t color_proton = { 1.0f, 0.27f, 0.22f, 1.0f };
static const color4_t color_neutron = { 0.32f, 0.61f, 1.0f, 1.0f };
static const color4_t color_electron = { 0.20f, 0.86f, 0.96f, 1.0f };

void editor_set_notice(app_t* app, const char* message) {
  (void)snprintf(app->notice, sizeof(app->notice), "%s", message);
  app->notice_time = 2.4f;
}

/**
 * @brief Returns the interface color associated with the current tool.
 *
 * @param tool Active editor tool.
 *
 * @return Color for the tool indicator: blue for neutron, cyan for
 *         electron, red for proton, and white for selection/unknown tools.
 */
static color4_t editor_tool_color(editor_tool_t tool) {
  switch (tool) {
    case editor_tool_neutron: return color_neutron;
    case editor_tool_electron: return color_electron;
    case editor_tool_proton: return color_proton;
    case editor_tool_select:
    default: return color_white;
  }
}

/* TODO description */
static void editor_update_title(app_t* app, float delta_time) {
  app->title_update_time -= delta_time;
  if (app->title_update_time > 0.0f) {
    return;
  }
  app->title_update_time = 0.4f;
  char title[192];
  (void)snprintf(
    title,
    sizeof(title),
    "atom forge | %s | particles %zu | atoms %zu | molecules %zu | zoom %.0f%%",
    app->world.paused ? "paused" : "running",
    world_particle_count(&app->world),
    app->world.atom_count,
    app->world.molecule_count,
    app->zoom / 42.0f * 100.0f
  );
  SDL_SetWindowTitle(app->window, title);
}

/* TODO description */
static const atom_t* editor_selected_atom(const app_t* app) {
  const particle_t* selected = world_get_particle_const(
    &app->world,
    app->world.selected_particle
  );
  if (selected == NULL) {
    return NULL;
  }
  if (selected->kind == particle_electron) {
    int nearest = -1;
    float nearest_distance_sq = 2.65f * 2.65f;
    for (size_t i = 0; i < app->world.atom_count; ++i) {
      float distance_sq = vec2_dist2(
        selected->position,
        app->world.atoms[i].position
      );
      if (distance_sq <= nearest_distance_sq) {
        nearest = (int)i;
        nearest_distance_sq = distance_sq;
      }
    }
    return nearest >= 0 ? &app->world.atoms[nearest] : NULL;
  }
  for (size_t i = 0; i < app->world.atom_count; ++i) {
    const atom_t* atom = &app->world.atoms[i];
    for (size_t j = 0; j < atom->member_count; ++j) {
      if ((int)atom->member_slots[j] == app->world.selected_particle) {
        return atom;
      }
    }
  }
  return NULL;
}

/* TODO description */
static void editor_draw_hud(const app_t* app) {
  renderer_t* renderer = app->renderer;
  color4_t tool_color = editor_tool_color(app->world.tool);
  renderer_draw_rect(renderer, 14.0f, 14.0f, 570.0f, 48.0f, color_panel);
  renderer_draw_rect(renderer, 14.0f, 14.0f, 3.0f, 48.0f, tool_color);
  renderer_draw_rect(renderer, 17.0f, 14.0f, 567.0f, 1.0f, color_panel_edge);
  renderer_draw_text(
    renderer,
    27.0f,
    22.0f,
    "ATOM FORGE   1 P+  2 N  3 E-  0 SELECT",
    2,
    color_white
  );
  renderer_draw_text(
    renderer,
    27.0f,
    45.0f,
    "SPACE RUN/PAUSE | LMB PLACE/MOVE | RMB DELETE | WHEEL ZOOM | MMB PAN",
    1,
    color_muted
  );

  float panel_y = (float)app->height - 66.0f;
  renderer_draw_rect(renderer, 14.0f, panel_y, 590.0f, 52.0f, color_panel);
  renderer_draw_rect(renderer, 14.0f, panel_y, 590.0f, 1.0f, color_panel_edge);
  char stats[160];
  (void)snprintf(
    stats,
    sizeof(stats),
    "PART %zu  ATOMS %zu  BONDS %zu  MOLECULES %zu",
    world_particle_count(&app->world),
    app->world.atom_count,
    app->world.bond_count,
    app->world.molecule_count
  );
  renderer_draw_text(renderer, 25.0f, panel_y + 6.0f, stats, 1, color_white);
  char mode[128];
  (void)snprintf(
    mode,
    sizeof(mode),
    "%s | TOOL %s | ZOOM %.0f%% | CTRL+S SAVE / CTRL+L LOAD",
    app->world.paused ? "PAUSED" : "RUNNING",
    world_tool_name(app->world.tool),
    app->zoom / 42.0f * 100.0f
  );
  renderer_draw_text(renderer, 25.0f, panel_y + 21.0f, mode, 1,
    app->world.paused ? color_muted : color_electron);

  char selection[128];
  const particle_t* selected_particle = world_get_particle_const(
    &app->world,
    app->world.selected_particle
  );
  const atom_t* selected_atom = editor_selected_atom(app);
  if (selected_atom != NULL) {
    (void)snprintf(
      selection,
      sizeof(selection),
      "SEL %s Z%u N%u E%u Q%+d %s",
      selected_atom->symbol,
      selected_atom->proton_count,
      selected_atom->neutron_count,
      selected_atom->electron_count,
      selected_atom->charge,
      selected_atom->radioactive ? "RADIOACTIVE" : "STABLE"
    );
  } else if (selected_particle != NULL) {
    (void)snprintf(selection, sizeof(selection), "SEL %s - UNBOUND",
      world_particle_name(selected_particle->kind));
  } else {
    (void)snprintf(selection, sizeof(selection), "SELECT A PARTICLE FOR ATOM DETAILS");
  }
  renderer_draw_text(renderer, 25.0f, panel_y + 36.0f, selection, 1,
    selected_atom != NULL && selected_atom->radioactive ? color_warn : color_muted);

  if (app->notice_time > 0.0f) {
    float box_width = (float)strlen(app->notice) * 7.0f + 24.0f;
    float box_x = ((float)app->width - box_width) * 0.5f;
    renderer_draw_rect(renderer, box_x, 18.0f, box_width, 27.0f,
      (color4_t){ 0.04f, 0.06f, 0.08f, 0.95f });
    renderer_draw_rect(renderer, box_x, 18.0f, 2.0f, 27.0f, color_warn);
    renderer_draw_text(renderer, box_x + 12.0f, 27.0f, app->notice, 1, color_white);
  }
}

/* TODO description */
static void editor_draw_atom_labels(const app_t* app) {
  if (app->zoom < 22.0f) {
    return;
  }
  for (size_t i = 0; i < app->world.atom_count; ++i) {
    const atom_t* atom = &app->world.atoms[i];
    vec2_t screen = renderer_world_to_screen(app->renderer, atom->position);
    char label[12];
    (void)snprintf(label, sizeof(label), "%s%s", atom->symbol,
      atom->radioactive ? "!" : "");
    color4_t color = atom->radioactive ? color_warn : color_white;
    renderer_draw_text(app->renderer, screen.x + 7.0f, screen.y - 16.0f,
      label, 1, color);
  }
}

void editor_update(app_t* app, float delta_time) {
  if (app == NULL) {
    return;
  }
  if (app->notice_time > 0.0f) {
    app->notice_time = fmaxf(0.0f, app->notice_time - delta_time);
  }
  editor_update_title(app, delta_time);
}

void editor_draw_ui(const app_t* app) {
  if (app == NULL || app->renderer == NULL) {
    return;
  }
  editor_draw_atom_labels(app);
  editor_draw_hud(app);
}
