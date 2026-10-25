#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom_forge/chemistry.h"
#include "atom_forge/physics.h"
#include "atom_forge/vec2.h"
#include "atom_forge/world.h"

static void test_vec2(void) {
  vec2_t a = vec2(3.0f, 4.0f);
  vec2_t b = vec2(1.0f, 2.0f);
  vec2_t sum = vec2_add(a, b);
  assert(sum.x == 4.0f && sum.y == 6.0f);
  assert(fabsf(vec2_length(a) - 5.0f) < 1.0e-5f);
  assert(fabsf(vec2_distance(a, b) - sqrtf(8.0f)) < 1.0e-5f);
  assert(vec2_is_finite(vec2_normalize(a)));
  assert(vec2_length(vec2_clamp_length(a, 2.0f)) <= 2.0001f);
}

static void test_world_slots(void) {
  world_t world;
  world_init(&world);
  int proton = world_add_particle(&world, particle_proton, vec2(0.0f, 0.0f));
  int neutron = world_add_particle(&world, particle_neutron, vec2(1.0f, 0.0f));
  assert(proton == 0 && neutron == 1);
  assert(world_particle_count(&world) == 2);
  assert(world_find_particle_at(&world, vec2_make(0.1f, 0.0f), 0.5f) == proton);
  assert(world_remove_particle(&world, proton));
  assert(world_get_particle(&world, proton) == NULL);
  assert(world_particle_count(&world) == 1);
}

static void test_isotope_data(void) {
  assert(chemistry_isotope_stable(1, 0));
  assert(chemistry_isotope_stable(6, 6));
  assert(chemistry_isotope_stable(8, 8));
  assert(!chemistry_isotope_stable(6, 5));
  assert(!chemistry_isotope_stable(19, 21));
  assert(chemistry_neutral_valence(6) == 4);
}

static void test_carbon_composition(void) {
  world_t world;
  world_init(&world);
  for (int i = 0; i < 6; ++i) {
    (void)world_add_particle(&world, particle_proton,
      vec2((float)i * 0.1f, 0.0f));
    (void)world_add_particle(&world, particle_neutron,
      vec2((float)i * 0.1f, 0.1f));
  }
  for (int i = 0; i < 6; ++i) {
    (void)world_add_particle(&world, particle_electron,
      vec2(0.2f, 0.2f));
  }
  chemistry_rebuild(&world);
  assert(world.atom_count == 1);
  assert(world.atoms[0].proton_count == 6);
  assert(world.atoms[0].neutron_count == 6);
  assert(world.atoms[0].electron_count == 6);
  assert(world.atoms[0].charge == 0);
  assert(!world.atoms[0].radioactive);
  assert(world.atoms[0].symbol[0] == 'C');
}

static void test_hydrogen_molecule(void) {
  world_t world;
  world_init(&world);
  (void)world_add_particle(&world, particle_proton, vec2(-0.65f, 0.0f));
  (void)world_add_particle(&world, particle_electron, vec2(-0.65f, 0.0f));
  (void)world_add_particle(&world, particle_proton, vec2(0.65f, 0.0f));
  (void)world_add_particle(&world, particle_electron, vec2(0.65f, 0.0f));
  chemistry_rebuild(&world);
  assert(world.atom_count == 2);
  assert(world.bond_count == 1);
  assert(world.molecule_count == 1);
  assert(world.molecules[0].atom_count == 2);
}

static void test_save_load(void) {
  const char* path = "atom_forge_test.save";
  world_t original;
  world_t restored;
  world_init(&original);
  world_init(&restored);
  (void)world_add_particle(&original, particle_proton, vec2(1.25f, -2.5f));
  (void)world_add_particle(&original, particle_electron, vec2(2.0f, 3.0f));
  assert(world_save(&original, path));
  assert(world_load(&restored, path));
  assert(world_particle_count(&restored) == 2);
  assert(restored.particles[0].kind == particle_proton);
  assert(fabsf(restored.particles[0].position.x - 1.25f) < 1.0e-5f);
  assert(fabsf(restored.particles[0].position.y + 2.5f) < 1.0e-5f);
  remove(path);
}

static void test_physics_respects_pause(void) {
  world_t world;
  world_init(&world);
  int proton = world_add_particle(&world, particle_proton, vec2(0.0f, 0.0f));
  int neutron = world_add_particle(&world, particle_neutron, vec2(1.0f, 0.0f));
  vec2_t before = world.particles[neutron].position;
  physics_update(&world, 0.016f);
  assert(world.particles[neutron].position.x == before.x);
  world.paused = false;
  physics_update(&world, 0.016f);
  assert(world.particles[neutron].position.x != before.x);
  assert(world_get_particle(&world, proton) != NULL);
}

int main(void) {
  test_vec2();
  test_world_slots();
  test_isotope_data();
  test_carbon_composition();
  test_hydrogen_molecule();
  test_save_load();
  test_physics_respects_pause();
  puts("all atom_forge backend tests passed");
  return 0;
}
