#include <math.h>
#include <stdio.h>
#include <string.h>

#include "atom_forge/world.h"

/**
 * @brief Validates a particle kind.
 *
 * @param kind Particle type to validate.
 *
 * @return true for particle_proton, particle_neutron, or particle_electron;
 *         otherwise, false.
 */
static bool world_valid_particle_kind(particle_kind_t kind) {
  return kind == particle_proton || kind == particle_neutron ||
    kind == particle_electron;
}

void world_init(world_t* world) {
  if (world == NULL) {
    return;
  }
  memset(world, 0, sizeof(*world));
  world->selected_particle = -1;
  world->tool = editor_tool_select;
  world->paused = true;
}

void world_clear(world_t* world) {
  world_init(world);
}

int world_add_particle(world_t* world, particle_kind_t kind, vec2_t position) {
  if (world == NULL || !world_valid_particle_kind(kind) ||
      !vec2_is_finite(position)) {
    return -1;
  }
  for (int i = 0; i < max_particles; ++i) {
    particle_t* particle = &world->particles[i];
    if (particle->alive) {
      continue;
    }
    *particle = (particle_t){
      .alive = true,
      .kind = kind,
      .position = position,
      .velocity = { 0.0f, 0.0f },
      .decay_time = 0.0f,
      .flash_time = 0.0f
    };
    return i;
  }
  return -1;
}

bool world_remove_particle(world_t* world, int slot) {
  if (world == NULL || slot < 0 || slot >= max_particles ||
      !world->particles[slot].alive) {
    return false;
  }
  memset(&world->particles[slot], 0, sizeof(world->particles[slot]));
  if (world->selected_particle == slot) {
    world->selected_particle = -1;
  }
  return true;
}

particle_t* world_get_particle(world_t* world, int slot) {
  if (world == NULL || slot < 0 || slot >= max_particles ||
      !world->particles[slot].alive) {
    return NULL;
  }
  return &world->particles[slot];
}

const particle_t* world_get_particle_const(const world_t* world, int slot) {
  if (world == NULL || slot < 0 || slot >= max_particles ||
      !world->particles[slot].alive) {
    return NULL;
  }
  return &world->particles[slot];
}

int world_find_particle_at(const world_t* world, vec2_t position, float radius) {
  if (world == NULL || !vec2_is_finite(position) || !isfinite(radius) ||
      radius < 0.0f) {
    return -1;
  }
  int nearest_slot = -1;
  float nearest_distance_sq = radius * radius;
  for (int i = 0; i < max_particles; ++i) {
    const particle_t* particle = &world->particles[i];
    if (!particle->alive) {
      continue;
    }
    float distance_sq = vec2_dist2(position, particle->position);
    if (distance_sq <= nearest_distance_sq) {
      nearest_slot = i;
      nearest_distance_sq = distance_sq;
    }
  }
  return nearest_slot;
}

size_t world_particle_count(const world_t* world) {
  if (world == NULL) {
    return 0;
  }
  size_t count = 0;
  for (size_t i = 0; i < max_particles; ++i) {
    count += world->particles[i].alive ? 1u : 0u;
  }
  return count;
}

const char* world_tool_name(editor_tool_t tool) {
  switch (tool) {
    case editor_tool_proton: return "PROTON";
    case editor_tool_neutron: return "NEUTRON";
    case editor_tool_electron: return "ELECTRON";
    case editor_tool_select:
    default: return "SELECT";
  }
}

const char* world_particle_name(particle_kind_t kind) {
  switch (kind) {
    case particle_proton: return "PROTON";
    case particle_neutron: return "NEUTRON";
    case particle_electron: return "ELECTRON";
    default: return "UNKNOWN";
  }
}

char world_particle_code(particle_kind_t kind) {
  switch (kind) {
    case particle_proton: return 'p';
    case particle_neutron: return 'n';
    case particle_electron: return 'e';
    default: return '?';
  }
}

bool world_save(const world_t* world, const char* path) {
  if (world == NULL || path == NULL || path[0] == '\0') {
    return false;
  }
  FILE* file = fopen(path, "w");
  if (file == NULL) {
    return false;
  }
  bool success = fprintf(file, "# atom_forge_save_v1\n") >= 0;
  for (size_t i = 0; success && i < max_particles; ++i) {
    const particle_t* particle = &world->particles[i];
    if (!particle->alive) {
      continue;
    }
    success = fprintf(file, "%c %.6f %.6f\n",
      world_particle_code(particle->kind),
      (double)particle->position.x,
      (double)particle->position.y) >= 0;
  }
  if (fclose(file) != 0) {
    success = false;
  }
  return success;
}

bool world_load(world_t* world, const char* path) {
  if (world == NULL || path == NULL || path[0] == '\0') {
    return false;
  }
  FILE* file = fopen(path, "r");
  if (file == NULL) {
    return false;
  }

  particle_t loaded[max_particles] = { 0 };
  size_t count = 0;
  char line[160];
  bool valid = true;
  while (fgets(line, sizeof(line), file) != NULL) {
    const char* cursor = line;
    while (*cursor == ' ' || *cursor == '\t') {
      ++cursor;
    }
    if (*cursor == '\0' || *cursor == '\n' || *cursor == '#') {
      continue;
    }

    char code = '\0';
    char trailing = '\0';
    float x = 0.0f;
    float y = 0.0f;
    if (sscanf(cursor, " %c %f %f %c", &code, &x, &y, &trailing) != 3 ||
        !isfinite(x) || !isfinite(y) || count >= max_particles) {
      valid = false;
      break;
    }

    particle_kind_t kind;
    switch (code) {
      case 'p': kind = particle_proton; break;
      case 'n': kind = particle_neutron; break;
      case 'e': kind = particle_electron; break;
      default:
        valid = false;
        continue;
    }
    loaded[count++] = (particle_t){
      .alive = true,
      .kind = kind,
      .position = { x, y },
      .velocity = { 0.0f, 0.0f },
      .decay_time = 0.0f,
      .flash_time = 0.0f
    };
  }
  if (ferror(file)) {
    valid = false;
  }
  if (fclose(file) != 0) {
    valid = false;
  }
  if (!valid) {
    return false;
  }

  memset(world->particles, 0, sizeof(world->particles));
  memcpy(world->particles, loaded, count * sizeof(loaded[0]));
  world->atom_count = 0;
  world->bond_count = 0;
  world->molecule_count = 0;
  world->selected_particle = -1;
  world->tool = editor_tool_select;
  world->paused = true;
  world->simulation_time = 0.0f;
  return true;
}
