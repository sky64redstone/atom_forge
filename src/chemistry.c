#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom_forge/chemistry.h"


/**
 * @brief Finds the representative index of a disjoint-set group.
 *
 * @param parents Array mapping each item to its parent index.
 * @param slot Item whose group representative is requested.
 *
 * @return The root index of the item's group.
 *
 * @sideeffect Applies path compression by redirecting traversed items
 *             directly toward the root.
 *
 * @warning slot must be a valid array index whose parent chain is valid.
 */
static int chemistry_find_root(int* parents, int slot);

/**
 * @brief Joins the disjoint-set groups containing a and b.
 *
 * @param parents Disjoint-set parent array.
 * @param a First item index.
 * @param b Second item index.
 *
 * @return Nothing.
 *
 * @sideeffect If the items have different roots, makes the larger root
 *             index point to the smaller root index.
 *
 * @note This implementation uses deterministic root selection and
 *       path compression, but not union-by-rank.
 */
static void chemistry_union(int* parents, int a, int b);

/*
 * Private data structures and tables
 * - element_names[]: names indexed by proton count.
 * - element_symbols[]: element symbols indexed by proton count.
 * - neutral_valence[]: simplified valence capacities for neutral atoms.
 * - stable_isotopes[]: a list of proton/neutron count pairs marked stable.
 * The private stable_isotope_t type stores the atomic number through
 * proton_count and the isotope neutron count through neutron_count.
 *
 * The private bond_candidate_t structure stores two atom indices plus their
 * squared separation. It is used to sort potential chemical bonds before
 * the backend accepts them.
 */

/* These tables cover elements 1-20 and selected isotope stability data. */

static const char* element_names[max_elements + 1] = {
  "unknown",
  "hydrogen", "helium", "lithium", "beryllium", "boron",
  "carbon", "nitrogen", "oxygen", "fluorine", "neon",
  "sodium", "magnesium", "aluminium", "silicon", "phosphorus",
  "sulfur", "chlorine", "argon", "potassium", "calcium"
};

static const char* element_symbols[max_elements + 1] = {
  "?",
  "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne",
  "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca"
};

static const int neutral_valence[max_elements + 1] = {
  0, 1, 0, 1, 2, 3, 4, 3, 2, 1, 0,
  1, 2, 3, 4, 3, 2, 1, 0, 1, 2
};

typedef struct stable_isotope_range {
  unsigned int proton_count;
  unsigned int neutron_count;
} stable_isotope_t;

static const stable_isotope_t stable_isotopes[] = {
  { 1, 0 }, { 1, 1 },
  { 2, 1 }, { 2, 2 },
  { 3, 3 }, { 3, 4 },
  { 4, 5 },
  { 5, 5 }, { 5, 6 },
  { 6, 6 }, { 6, 7 },
  { 7, 7 }, { 7, 8 },
  { 8, 8 }, { 8, 9 }, { 8, 10 },
  { 9, 10 },
  { 10, 10 }, { 10, 11 }, { 10, 12 },
  { 11, 12 },
  { 12, 12 }, { 12, 13 }, { 12, 14 },
  { 13, 14 },
  { 14, 14 }, { 14, 15 }, { 14, 16 },
  { 15, 16 },
  { 16, 16 }, { 16, 17 }, { 16, 18 }, { 16, 20 },
  { 17, 18 }, { 17, 20 },
  { 18, 18 }, { 18, 20 }, { 18, 22 },
  { 19, 20 }, { 19, 22 },
  { 20, 20 }, { 20, 22 }, { 20, 23 }, { 20, 24 }, { 20, 26 }, { 20, 28 }
};

const char* chemistry_element_name(unsigned int proton_count) {
  if (proton_count == 0 || proton_count > max_elements) {
    return "unknown";
  }
  return element_names[proton_count];
}

const char* chemistry_element_symbol(unsigned int proton_count) {
  if (proton_count == 0 || proton_count > max_elements) {
    return "?";
  }
  return element_symbols[proton_count];
}

bool chemistry_isotope_stable(unsigned int proton_count, unsigned int neutron_count) {
  size_t isotope_count = sizeof(stable_isotopes) / sizeof(stable_isotopes[0]);
  for (size_t i = 0; i < isotope_count; ++i) {
    if (stable_isotopes[i].proton_count == proton_count &&
        stable_isotopes[i].neutron_count == neutron_count) {
      return true;
    }
  }
  return false;
}

int chemistry_neutral_valence(unsigned int proton_count) {
  if (proton_count == 0 || proton_count > max_elements) {
    return 0;
  }
  return neutral_valence[proton_count];
}

/*
 * 1. Look up the nominal neutral valence.
 * 2. Return zero if the element has zero conf*igured valence or has no electrons.
 * 3. Calculate the net charge, proton_count - electron_count.
 * 4. Reject atoms with charge of +2 or greater.
 * 5. Reject singly positive atoms whose base valence is one.
 * 6. For negatively charged atoms, adjust the base capacity by the negative charge.
 * 7. Clamp the final result to the range 0–4.
 *
 * Each element initially points to itself. When two elements are connected,
 * their root entries are joined. Path compression flattens the tree over
 * repeated lookups.
 *
 * The helper is used both when constructing nuclear groups and
 * when building molecule connectivity.
 */
int chemistry_valence_capacity(
  unsigned int proton_count,
  unsigned int electron_count
) {
  int base = chemistry_neutral_valence(proton_count);
  if (base == 0 || electron_count == 0 || proton_count > max_elements) {
    return 0;
  }
  int charge = (int)proton_count - (int)electron_count;
  if (charge >= 2) {
    return 0;
  }
  if (charge == 1 && base == 1) {
    return 0;
  }
  if (charge < 0) {
    base += charge;
  }
  if (base < 0) {
    base = 0;
  }
  if (base > 4) {
    base = 4;
  }
  return base;
}


static int chemistry_find_root(int* parents, int slot) {
  int root = slot;
  while (parents[root] != root) {
    root = parents[root];
  }
  while (parents[slot] != slot) {
    int next = parents[slot];
    parents[slot] = root;
    slot = next;
  }
  return root;
}

/*
 * The function casts the generic void* arguments provided by qsort() back to
 * bond_candidate_t* and compares their distance_sq fields.
 *
 * Sorting candidates by proximity gives the bond-building stage a useful
 * greedy rule: shorter potential bonds are considered before longer ones.
 */
static void chemistry_union(int* parents, int a, int b) {
  int root_a = chemistry_find_root(parents, a);
  int root_b = chemistry_find_root(parents, b);
  if (root_a == root_b) {
    return;
  }
  if (root_a < root_b) {
    parents[root_b] = root_a;
  } else {
    parents[root_a] = root_b;
  }
}

typedef struct bond_candidate {
  uint16_t atom_a;
  uint16_t atom_b;
  float distance_sq;
} bond_candidate_t;

static int chemistry_compare_bond_candidates(const void* left, const void* right) {
  const bond_candidate_t* a = (const bond_candidate_t*)left;
  const bond_candidate_t* b = (const bond_candidate_t*)right;
  if (a->distance_sq < b->distance_sq) {
    return -1;
  }
  if (a->distance_sq > b->distance_sq) {
    return 1;
  }
  return 0;
}

/*
 * Stage 1: Group nucleons into nuclei.
 *
 * - Each live proton and neutron begins in its own disjoint-set group.
 * - Electrons are excluded from nuclear clustering.
 * - Every pair of live nucleons is checked.
 * - Nucleons within 1.15 world units are joined into the same group.
 * The connections are transitive. If A is close to B and B is close to C,
 * all three become a single group even if A and C are not directly within
 * the threshold.
 *
 * Stage 2: Construct atom records.
 *
 * The previous atom, bond, and molecule arrays are reset.
 * Each nucleon is assigned to the atom corresponding to its group root.
 * The backend calculates the group's proton count, neutron count,
 * mean position, root slot, and member slots. Groups without protons are
 * removed from the atom results.
 * For each remaining atom, the element symbol and name are derived from its
 * proton count, and its radioactive flag is initially determined from
 * chemistry_isotope_stable().
 *
 * Stage 3: Assign electrons.
 *
 * Each live electron is assigned to the closest atom whose center is within
 * 2.65 world units. An electron outside every atom's binding radius is left
 * unassigned.
 *
 * The charge is then calculated:
 * atom->charge = (int)atom->proton_count - (int)atom->electron_count;
 *
 * The backend also calculates valence_capacity using:
 * chemistry_valence_capacity()
 *
 * Stage 4: Infer chemical bonds.
 *
 * Every pair of atoms with positive valence capacity is checked. Pairs
 * separated by no more than 2.85 world units become bond candidates.
 * The candidate list is sorted by distance. The backend accepts candidates in
 * that order if both atoms have remaining capacity and the fixed bond array
 * still has space.
 * Accepted bonds update the bond array and increment the bond_count of both
 * participating atoms.
 *
 * Stage 5: Construct molecules.
 *
 * A second union-find pass joins atoms using the accepted bond edges.
 * Connected bonded atoms receive the same molecule ID, and their positions are
 * averaged into a molecule center.
 * An atom with no accepted bonds does not become an entry in world->molecules.
 * It remains an isolated atom.
 *
 * Important caveats in this function
 * There are some limitations worth knowing when reading or extending this code
 * - The algorithm uses all-pairs distance checks, which are acceptable for a
 *     256-particle limit but would need a spatial acceleration structure for
 *     much larger worlds.
 * - Stability is determined by an incomplete table. An unlisted isotope is
 *     treated as radioactive.
 * - Electron assignment is greedy: each electron goes to its nearest atom and
 *     is not distributed to minimize a global energy.
 * - Bond selection is distance-based and greedy.
 *     It does not optimize a global chemical structure.
 * - decay_phase is reset during each rebuild, while the root particle's
 *     decay_time stores the actual running decay timer. This distinction
 *     matters for changes to the UI or decay display.
 */
void chemistry_rebuild(world_t* world) {
  if (world == NULL) {
    return;
  }

  const float nuclear_bond_distance_sq = 1.15f * 1.15f;
  const float electron_binding_distance_sq = 2.65f * 2.65f;
  const float chemical_bond_distance_sq = 2.85f * 2.85f;
  int parents[max_particles];
  int root_to_atom[max_particles];
  memset(root_to_atom, 0xff, sizeof(root_to_atom));

  for (int i = 0; i < max_particles; ++i) {
    const particle_t* particle = &world->particles[i];
    parents[i] = (particle->alive && particle->kind != particle_electron) ? i : -1;
  }

  for (int i = 0; i < max_particles; ++i) {
    const particle_t* a = &world->particles[i];
    if (!a->alive || a->kind == particle_electron) {
      continue;
    }
    for (int j = i + 1; j < max_particles; ++j) {
      const particle_t* b = &world->particles[j];
      if (!b->alive || b->kind == particle_electron) {
        continue;
      }
      if (vec2_dist2(a->position, b->position) <= nuclear_bond_distance_sq) {
        chemistry_union(parents, i, j);
      }
    }
  }

  world->atom_count = 0;
  world->bond_count = 0;
  world->molecule_count = 0;
  memset(world->atoms, 0, sizeof(world->atoms));
  memset(world->bonds, 0, sizeof(world->bonds));
  memset(world->molecules, 0, sizeof(world->molecules));

  for (int i = 0; i < max_particles; ++i) {
    const particle_t* particle = &world->particles[i];
    if (!particle->alive || particle->kind == particle_electron) {
      continue;
    }
    int root = chemistry_find_root(parents, i);
    int atom_index = root_to_atom[root];
    if (atom_index < 0) {
      atom_index = (int)world->atom_count++;
      root_to_atom[root] = atom_index;
      atom_t* atom = &world->atoms[atom_index];
      atom->root_slot = root;
      atom->molecule_id = -1;
      atom->position = vec2(0.0f, 0.0f);
    }
    atom_t* atom = &world->atoms[atom_index];
    atom->position = vec2_add(atom->position, particle->position);
    if (particle->kind == particle_proton) {
      ++atom->proton_count;
    } else {
      ++atom->neutron_count;
    }
    if (atom->member_count < max_particles) {
      atom->member_slots[atom->member_count++] = (uint16_t)i;
    }
  }

  size_t write_index = 0;
  for (size_t i = 0; i < world->atom_count; ++i) {
    atom_t* atom = &world->atoms[i];
    if (atom->proton_count == 0) {
      continue;
    }
    atom->position = vec2_scale(atom->position, 1.0f / (float)atom->member_count);
    atom->charge = (int)atom->proton_count;
    atom->radioactive = !chemistry_isotope_stable(
      atom->proton_count,
      atom->neutron_count
    );
    atom->decay_phase = 0.0f;
    (void)snprintf(atom->symbol, sizeof(atom->symbol), "%s",
      chemistry_element_symbol(atom->proton_count));
    (void)snprintf(atom->name, sizeof(atom->name), "%s",
      chemistry_element_name(atom->proton_count));
    if (write_index != i) {
      world->atoms[write_index] = *atom;
    }
    ++write_index;
  }
  world->atom_count = write_index;

  for (int i = 0; i < max_particles; ++i) {
    const particle_t* particle = &world->particles[i];
    if (!particle->alive || particle->kind != particle_electron) {
      continue;
    }
    int closest_atom = -1;
    float closest_distance_sq = electron_binding_distance_sq;
    for (size_t a = 0; a < world->atom_count; ++a) {
      float distance_sq = vec2_dist2(
        particle->position,
        world->atoms[a].position
      );
      if (distance_sq <= closest_distance_sq) {
        closest_distance_sq = distance_sq;
        closest_atom = (int)a;
      }
    }
    if (closest_atom >= 0) {
      ++world->atoms[closest_atom].electron_count;
    }
  }

  for (size_t i = 0; i < world->atom_count; ++i) {
    atom_t* atom = &world->atoms[i];
    atom->charge = (int)atom->proton_count - (int)atom->electron_count;
    atom->valence_capacity = chemistry_valence_capacity(
      atom->proton_count,
      atom->electron_count
    );
  }

  size_t candidate_count = 0;
  bond_candidate_t candidates[max_particles * (max_particles - 1) / 2];
  for (size_t i = 0; i < world->atom_count; ++i) {
    const atom_t* a = &world->atoms[i];
    if (a->valence_capacity <= 0) {
      continue;
    }
    for (size_t j = i + 1; j < world->atom_count; ++j) {
      const atom_t* b = &world->atoms[j];
      if (b->valence_capacity <= 0) {
        continue;
      }
      float distance_sq = vec2_dist2(a->position, b->position);
      if (distance_sq <= chemical_bond_distance_sq) {
        candidates[candidate_count++] = (bond_candidate_t){
          .atom_a = (uint16_t)i,
          .atom_b = (uint16_t)j,
          .distance_sq = distance_sq
        };
      }
    }
  }
  qsort(
    candidates, candidate_count, sizeof(candidates[0]),
    chemistry_compare_bond_candidates
  );

  for (size_t i = 0; i < candidate_count && world->bond_count < max_bonds; ++i) {
    bond_candidate_t candidate = candidates[i];
    atom_t* a = &world->atoms[candidate.atom_a];
    atom_t* b = &world->atoms[candidate.atom_b];
    if (a->bond_count >= a->valence_capacity ||
        b->bond_count >= b->valence_capacity) {
      continue;
    }
    world->bonds[world->bond_count++] = (chemical_bond_t){
      .atom_a = candidate.atom_a,
      .atom_b = candidate.atom_b,
      .distance_sq = candidate.distance_sq
    };
    ++a->bond_count;
    ++b->bond_count;
  }

  int molecule_parents[max_particles];
  int root_to_molecule[max_particles];
  memset(root_to_molecule, 0xff, sizeof(root_to_molecule));
  for (size_t i = 0; i < world->atom_count; ++i) {
    molecule_parents[i] = (int)i;
  }
  for (size_t i = 0; i < world->bond_count; ++i) {
    chemistry_union(molecule_parents, world->bonds[i].atom_a, world->bonds[i].atom_b);
  }
  for (size_t i = 0; i < world->bond_count; ++i) {
    uint16_t atom_index = world->bonds[i].atom_a;
    int root = chemistry_find_root(molecule_parents, atom_index);
    if (root_to_molecule[root] < 0) {
      int molecule_index = (int)world->molecule_count++;
      root_to_molecule[root] = molecule_index;
      world->molecules[molecule_index].position = vec2(0.0f, 0.0f);
    }
  }
  for (size_t i = 0; i < world->atom_count; ++i) {
    if (world->atoms[i].bond_count == 0) {
      continue;
    }
    int root = chemistry_find_root(molecule_parents, (int)i);
    int molecule_index = root_to_molecule[root];
    if (molecule_index >= 0) {
      molecule_t* molecule = &world->molecules[molecule_index];
      molecule->position = vec2_add(molecule->position, world->atoms[i].position);
      ++molecule->atom_count;
      world->atoms[i].molecule_id = molecule_index;
    }
  }
  for (size_t i = 0; i < world->molecule_count; ++i) {
    molecule_t* molecule = &world->molecules[i];
    if (molecule->atom_count > 0) {
      molecule->position = vec2_scale(
        molecule->position,
        1.0f / (float)molecule->atom_count
      );
    }
  }
}
