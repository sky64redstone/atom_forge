#include <math.h>
#include <stddef.h>

#include "atom_forge/physics.h"
#include "atom_forge/chemistry.h"

/**
 * @brief Returns the prototype's electric charge for a particle kind.
 *
 * @param kind Particle kind.
 *
 * @return +1 for a proton, -1 for an electron, and 0 for a neutron
 *         or an unrecognized enum value.
 *
 * @note The returned charge is dimensionless.
 */
static float physics_particle_charge(particle_kind_t kind) {
  switch (kind) {
    case particle_proton: return 1.0f;
    case particle_electron: return -1.0f;
    case particle_neutron: return 0.0f;
    default: return 0.0f;
  }
}

/**
 * @brief Produces a usable direction vector, including when the input
 *        displacement is effectively zero.
 *
 * @param delta Displacement vector from which to derive a direction.
 * @param seed Integer used to construct a deterministic fallback angle.
 *
 * @return A unit direction vector.
 *
 * @note If delta has sufficient length, returns its normalized direction.
 *       Otherwise, creates a deterministic direction from seed.
 * @note When two particles overlap, normalizing the displacement would produce
 *       a zero vector. This helper uses a deterministic fallback angle so
 *       coincident particles can still receive opposing forces or
 *       be moved apart.
 * @note The fallback angle is derived from the seed rather than a random-number
 *       generator, so a given seed produces the same direction every time.
 */
static vec2_t physics_safe_direction(vec2_t delta, int seed) {
  vec2_t direction = vec2_norm(delta);
  if (vec2_len2(direction) > 0.5f) {
    return direction;
  }
  float angle = (float)(seed * 37 + 11) * 0.6180339f;
  return vec2(cosf(angle), sinf(angle));
}

/**
 * @brief Finds a particle of a given kind among an atom's nucleons.
 *
 * @param world World containing the particles.
 * @param atom Atom whose member_slots array is searched.
 * @param kind Required particle kind.
 * @param avoid_slot Slot that must not be returned; use -1 to avoid none.
 *
 * @return Matching particle slot, or -1 if no match exists.
 *
 * @note Only searches the specified atom's member_slots list.
 * @note The decay routines use this function to locate a neutron or proton
 *       belonging to the nucleus. The avoid_slot argument allows them to
 *       prefer a particle other than the atom's chosen root particle.
 */
static int physics_find_member(
  const world_t* world,
  const atom_t* atom,
  particle_kind_t kind,
  int avoid_slot
) {
  for (size_t i = 0; i < atom->member_count; ++i) {
    int slot = (int)atom->member_slots[i];
    const particle_t* particle = world_get_particle_const(world, slot);
    if (particle != NULL && particle->kind == kind && slot != avoid_slot) {
      return slot;
    }
  }
  return -1;
}

/**
 * @brief Finds the nearest live electron within a maximum distance
 *        of a supplied center point.
 *
 * @param world World to search.
 * @param center Search center in world coordinates.
 * @param max_distance Maximum permitted distance.
 *
 * @return The slot of the nearest electron within the distance,
 *         or -1 if none is found.
 *
 * @note The search uses squared distances and includes the boundary.
 * @note Unlike physics_find_member(), this function searches all live
 *       particles of electron kind. It is used by the electron-capture rule.
 */
static int physics_find_nearby_electron(
  const world_t* world,
  vec2_t center,
  float max_distance
) {
  int result = -1;
  float best_distance_sq = max_distance * max_distance;
  for (int i = 0; i < max_particles; ++i) {
    const particle_t* particle = &world->particles[i];
    if (!particle->alive || particle->kind != particle_electron) {
      continue;
    }
    float distance_sq = vec2_dist2(center, particle->position);
    if (distance_sq <= best_distance_sq) {
      result = i;
      best_distance_sq = distance_sq;
    }
  }
  return result;
}

/**
 * @brief Attempts the prototype's beta-minus decay transformation.
 *
 * @param world World to modify.
 * @param atom Atom record describing the decaying nucleus.
 *
 * @return true if the transformation succeeds; otherwise, false.
 *
 * @note Converts one member neutron into a proton and creates an electron
 *       outside the nucleus, with an initial velocity and visual flash.
 * @note Returns false if there is no suitable neutron or no free slot
 *       for the emitted electron.
 * @note The routine first checks whether the world has room for another
 *       particle. It then locates a neutron in the atom and converts it to a
 *       proton. The emitted electron is placed outside the nucleus along a
 *       direction based on the neutron's pos*ition relative to the atom center.
 *       It is assigned outward velocity and a flash timer for visual feedback.
 *       If adding the electron fails, the neutron is restored to neutron kind
 *       and the routine reports failure.
 * @note This implementation does not model the emitted antineutrino or enforce
 *       a physical decay energy.
 */
static bool physics_beta_minus(world_t* world, const atom_t* atom) {
  if (world_particle_count(world) >= max_particles) {
    return false;
  }
  int neutron_slot = physics_find_member(
    world,
    atom,
    particle_neutron,
    atom->root_slot
  );
  if (neutron_slot < 0) {
    neutron_slot = physics_find_member(world, atom, particle_neutron, -1);
  }
  particle_t* neutron = world_get_particle(world, neutron_slot);
  if (neutron == NULL || neutron->kind != particle_neutron) {
    return false;
  }

  neutron->kind = particle_proton;
  neutron->velocity = vec2(0.0f, 0.0f);
  neutron->decay_time = 0.0f;
  vec2_t direction = physics_safe_direction(
    vec2_sub(neutron->position, atom->position),
    atom->root_slot
  );
  vec2_t electron_position = vec2_add(atom->position, vec2_scale(direction, 3.7f));
  int electron_slot = world_add_particle(world, particle_electron, electron_position);
  if (electron_slot < 0) {
    neutron->kind = particle_neutron;
    return false;
  }
  particle_t* electron = world_get_particle(world, electron_slot);
  if (electron != NULL) {
    electron->velocity = vec2_scale(direction, 1.7f);
    electron->flash_time = 0.8f;
  }
  return true;
}

/**
 * @brief Attempts the prototype's electron-capture transformation.
 *
 * @param world World to modify.
 * @param atom Atom record describing the decaying nucleus.
 *
 * @return true if a proton was converted successfully; otherwise, false.
 *
 * @sideeffect Removes a nearby electron and converts one member proton
 *             into a neutron, resetting the changed particle's velocity
 *             and decay timer.
 *
 * @note Returns false if no suitable proton or nearby electron exists.
 * @note The routine locates a proton within the nucleus and an electron within
 *       2.65 world units of the atom center. If both exist, the electron is
 *       removed and the proton changes type to neutron.
 */
static bool physics_electron_capture(world_t* world, const atom_t* atom) {
  int proton_slot = physics_find_member(
    world,
    atom,
    particle_proton,
    atom->root_slot
  );
  if (proton_slot < 0) {
    proton_slot = physics_find_member(world, atom, particle_proton, -1);
  }
  particle_t* proton = world_get_particle(world, proton_slot);
  if (proton == NULL || proton->kind != particle_proton) {
    return false;
  }
  int electron_slot = physics_find_nearby_electron(world, atom->position, 2.65f);
  if (electron_slot < 0) {
    return false;
  }
  (void)world_remove_particle(world, electron_slot);
  proton->kind = particle_neutron;
  proton->velocity = vec2(0.0f, 0.0f);
  proton->decay_time = 0.0f;
  return true;
}

/**
 * @brief Attempts the prototype's alpha-decay transformation.
 *
 * @param world World to modify.
 * @param atom Atom record describing the decaying nucleus.
 *
 * @return true if four nucleons can be selected and moved;
 *         otherwise, false.
 *
 * @note Moves two protons and two neutrons to a new location outside the
 *       original nucleus, resets their velocities and decay timers, and
 *       triggers visual flashes.
 * @note This models the emitted alpha particle by repositioning four
 *       existing particles rather than allocating new particles.
 * @note The routine refuses to proceed unless the atom has at least three
 *       protons and three neutrons, leaving at least one proton and one
 *       neutron in the original group according to those count checks.
 * @note It selects two protons and two neutrons other than the root slot,
 *       computes an offset center, and moves tho*se four particles near that
 *       center. The chemistry rebuild afterward determines whether they form
 *       their own atom group.
 * @note The routine does not calculate the energy release or enforce realistic
 *       alpha-particle emission kinematics.
 */
static bool physics_alpha_decay(world_t* world, const atom_t* atom) {
  if (atom->proton_count < 3 || atom->neutron_count < 3) {
    return false;
  }
  int protons[2] = { -1, -1 };
  int neutrons[2] = { -1, -1 };
  for (size_t i = 0; i < atom->member_count; ++i) {
    int slot = (int)atom->member_slots[i];
    const particle_t* particle = world_get_particle_const(world, slot);
    if (particle == NULL || slot == atom->root_slot) {
      continue;
    }
    if (particle->kind == particle_proton && protons[0] < 0) {
      protons[0] = slot;
    } else if (particle->kind == particle_proton && protons[1] < 0) {
      protons[1] = slot;
    } else if (particle->kind == particle_neutron && neutrons[0] < 0) {
      neutrons[0] = slot;
    } else if (particle->kind == particle_neutron && neutrons[1] < 0) {
      neutrons[1] = slot;
    }
  }
  if (protons[0] < 0 || protons[1] < 0 || neutrons[0] < 0 || neutrons[1] < 0) {
    return false;
  }

  vec2_t direction = physics_safe_direction(
    vec2_sub(atom->position, vec2(0.0f, 0.0f)),
    atom->root_slot
  );
  vec2_t alpha_center = vec2_add(atom->position, vec2_scale(direction, 3.2f));
  const vec2_t offsets[4] = {
    { -0.18f, -0.18f }, { 0.18f, -0.18f },
    { -0.18f, 0.18f }, { 0.18f, 0.18f }
  };
  const int slots[4] = { protons[0], protons[1], neutrons[0], neutrons[1] };
  for (size_t i = 0; i < 4; ++i) {
    particle_t* particle = world_get_particle(world, slots[i]);
    if (particle == NULL) {
      continue;
    }
    particle->position = vec2_add(alpha_center, offsets[i]);
    particle->velocity = vec2(0.0f, 0.0f);
    particle->decay_time = 0.0f;
    particle->flash_time = 1.0f;
  }
  return true;
}

/**
 * @brief Selects and attempts a simplified decay mode based on
 *        the current atom composition.
 *
 * @param world World to modify.
 * @param atom Atom eligible for a decay attempt.
 *
 * @return true if one of the supported decay routines succeeds;
 *         otherwise, false.
 *
 * @note May modify particle types, remove an electron, add an electron,
 *       or move nucleons.
 * @note Tries beta-minus first when neutron count exceeds proton count,
 *       electron capture when proton count is at least neutron count,
 *       and alpha decay as the final fallback.
 */
static bool physics_attempt_decay(world_t* world, const atom_t* atom) {
  if (atom->neutron_count > atom->proton_count && atom->neutron_count > 0) {
    if (physics_beta_minus(world, atom)) {
      return true;
    }
  }
  if (atom->proton_count >= atom->neutron_count) {
    if (physics_electron_capture(world, atom)) {
      return true;
    }
  }
  return physics_alpha_decay(world, atom);
}

/**
 * @brief Calculates pair forces and advances all live particle positions.
 *
 * @param world World to simulate.
 * @param delta_time Time step in seconds of game time.
 *
 * @return Nothing.
 *
 * @note Changes particle velocities and positions and decrements positive
 *       visual flash timers.
 * @note The routine implements a simplified softened force model
 *       and damping, not real nuclear dynamics.
 */
static void physics_integrate(world_t* world, float delta_time) {
  vec2_t forces[max_particles];
  for (size_t i = 0; i < max_particles; ++i) {
    forces[i] = vec2(0.0f, 0.0f);
  }

  const float nuclear_capture_radius = 1.8f;
  const float nuclear_rest_distance = 0.56f;
  const float nuclear_spring = 6.5f;
  const float electrostatic_strength = 0.25f;
  const float softening_sq = 0.16f;

  for (int i = 0; i < max_particles; ++i) {
    const particle_t* a = &world->particles[i];
    if (!a->alive) {
      continue;
    }
    for (int j = i + 1; j < max_particles; ++j) {
      const particle_t* b = &world->particles[j];
      if (!b->alive) {
        continue;
      }
      vec2_t delta = vec2_sub(b->position, a->position);
      float distance_sq = vec2_len2(delta);
      float distance = 0.0f;
      if (distance_sq < 1.0e-6f) {
        delta = physics_safe_direction(delta, i * max_particles + j);
        distance = 0.05f;
        distance_sq = distance * distance;
      } else {
        distance = sqrtf(distance_sq);
      }
      vec2_t direction = vec2_norm(delta);
      vec2_t force = vec2(0.0f, 0.0f);

      bool a_nucleon = a->kind != particle_electron;
      bool b_nucleon = b->kind != particle_electron;
      if (a_nucleon && b_nucleon && distance < nuclear_capture_radius) {
        float magnitude = nuclear_spring * (distance - nuclear_rest_distance);
        magnitude = fmaxf(-7.0f, fminf(7.0f, magnitude));
        force = vec2_add(force, vec2_scale(direction, magnitude));
      }

      float charge_product = physics_particle_charge(a->kind) *
        physics_particle_charge(b->kind);
      if (charge_product != 0.0f) {
        float magnitude = -electrostatic_strength * charge_product /
          (distance_sq + softening_sq);
        magnitude = fmaxf(-2.0f, fminf(2.0f, magnitude));
        force = vec2_add(force, vec2_scale(direction, magnitude));
      }
      forces[i] = vec2_add(forces[i], force);
      forces[j] = vec2_sub(forces[j], force);
    }
  }

  float damping = expf(-2.4f * delta_time);
  for (int i = 0; i < max_particles; ++i) {
    particle_t* particle = &world->particles[i];
    if (!particle->alive) {
      continue;
    }
    float mass = particle->kind == particle_electron ? 0.55f : 1.0f;
    particle->velocity = vec2_add(
      particle->velocity,
      vec2_scale(forces[i], delta_time / mass)
    );
    particle->velocity = vec2_scale(particle->velocity, damping);
    particle->velocity = vec2_clamp_length(particle->velocity, 4.0f);
    particle->position = vec2_add(
      particle->position,
      vec2_scale(particle->velocity, delta_time)
    );
    if (particle->flash_time > 0.0f) {
      particle->flash_time = fmaxf(0.0f, particle->flash_time - delta_time);
    }
  }
}

void physics_update(world_t* world, float delta_time) {
  if (world == NULL) {
    return;
  }
  if (delta_time < 0.0f) {
    delta_time = 0.0f;
  }
  if (delta_time > 0.05f) {
    delta_time = 0.05f;
  }

  if (world->paused) {
    return;
  }

  world->simulation_time += delta_time;
  physics_integrate(world, delta_time);
  chemistry_rebuild(world);

  bool decayed = false;
  for (size_t i = 0; i < world->atom_count; ++i) {
    atom_t* atom = &world->atoms[i];
    particle_t* root = world_get_particle(world, atom->root_slot);
    if (root == NULL) {
      continue;
    }
    if (!atom->radioactive) {
      root->decay_time = 0.0f;
      continue;
    }
    root->decay_time += delta_time;
    atom->decay_phase = root->decay_time;
    float decay_delay = 3.8f + (float)(atom->root_slot % 7) * 0.42f;
    if (root->decay_time >= decay_delay) {
      if (physics_attempt_decay(world, atom)) {
        decayed = true;
      }
      root->decay_time = 0.0f;
    }
  }

  if (decayed) {
    chemistry_rebuild(world);
  }
}
