/*
 * Header: include/atom_forge/world.h
 * Implementation: src/world.c
 *
 * The world is the authoritative storage layer. It owns particle slots and
 * stores the calculated atom, bond, and molecule arrays. The chemistry backend
 * rebuilds those derived arrays whenever the particle configuration changes.
 */

#ifndef ATOM_FORGE_WORLD_H
  #define ATOM_FORGE_WORLD_H

  #include <stdbool.h>
  #include <stddef.h>
  #include <stdint.h>

  #include "atom_forge/vec2.h"

  /* Fixed capacities keep world storage predictable and allocation-free */
  enum {
    max_particles = 256,
    max_bonds = 512,
    max_elements = 20
  };

  typedef enum particle_kind {
    particle_proton,
    particle_neutron,
    particle_electron
  } particle_kind_t;

  typedef enum editor_tool {
    editor_tool_select,
    editor_tool_proton,
    editor_tool_neutron,
    editor_tool_electron
  } editor_tool_t;

  typedef struct particle {
    bool alive;
    particle_kind_t kind;
    vec2_t position;
    vec2_t velocity;
    float decay_time;
    float flash_time;
  } particle_t;

  typedef struct atom {
    int root_slot;
    vec2_t position;
    unsigned int proton_count;
    unsigned int neutron_count;
    unsigned int electron_count;
    int charge;
    int valence_capacity;
    int bond_count;
    int molecule_id;
    bool radioactive;
    float decay_phase;
    char symbol[4];
    char name[20];
    size_t member_count;
    uint16_t member_slots[max_particles];
  } atom_t;

  typedef struct chemical_bond {
    uint16_t atom_a;
    uint16_t atom_b;
    float distance_sq;
  } chemical_bond_t;

  typedef struct molecule {
    vec2_t position;
    unsigned int atom_count;
  } molecule_t;

  /* Particles are authoritative; atoms, bonds, and molecules are derived. */
  typedef struct world {
    particle_t particles[max_particles];
    size_t atom_count;
    atom_t atoms[max_particles];
    size_t bond_count;
    chemical_bond_t bonds[max_bonds];
    size_t molecule_count;
    molecule_t molecules[max_particles];
    int selected_particle;
    editor_tool_t tool;
    bool paused;
    float simulation_time;
  } world_t;

  /**
   * @brief Initializes a world to its default empty state.
   *
   * @param world World object to initialize.
   *
   * @return Nothing.
   *
   * @note A null pointer is ignored.
   * @note New worlds start paused in select mode.
   * @note Clears the entire structure, selects the select tool,
   *           clears selection, and starts the world paused.
   */
  void world_init(world_t* world);

  /**
   * @brief Resets a world to its initial empty state.
   *
   * @param world World object to clear.
   *
   * @return Nothing.
   *
   * @note Clears particles, atoms, bonds, molecules, simulation time,
   *           and editor selection by calling world_init().
   */
  void world_clear(world_t* world);

  /**
   * @brief Adds a particle to the first available slot.
   *
   * @param world Target world.
   * @param kind Type of particle to create.
   * @param position Initial position in world coordinates.
   *
   * @return The allocated particle slot, in [0, max_particles - 1],
   *         or -1 if the world pointer, kind, or position is invalid,
   *         or if no free slot exists.
   *
   * @note The caller must rebuild chemistry after adding particles
   *           if the derived atom and molecule data must be current.
   * @note Initializes the particle with zero velocity and zero
   *           decay/flash timers.
   */
  int world_add_particle(world_t* world, particle_kind_t kind, vec2_t position);

  /**
   * @brief Removes a particle from its slot.
   *
   * @param world World containing the particle.
   * @param slot Particle slot to remove.
   *
   * @return true if a live particle was removed; otherwise, false.
   *
   * @note Does not automatically rebuild chemistry.
   * @note Clears the slot and clears world selection if the removed
   *           particle was selected.
   */
  bool world_remove_particle(world_t* world, int slot);

  /**
   * @brief Returns a mutable pointer to a live particle.
   *
   * @param world World containing the particle.
   * @param slot Particle slot to retrieve.
   *
   * @return Pointer to the live particle, or NULL if the world or slot
   *         is invalid, or the slot is empty.
   *
   * @note the returned pointer allows the caller to modify the particle.
   */
  particle_t* world_get_particle(world_t* world, int slot);

  /**
   * @brief Returns a read-only pointer to a live particle.
   *
   * @param world World containing the particle.
   * @param slot Particle slot to retrieve.
   *
   * @return Pointer to the live particle, or NULL if the world or slot
   *         is invalid, or the slot is empty.
   *
   * @note The returned pointer cannot be used to modify the particle
   *       through this interface. (see: world_get_particle)
   */
  const particle_t* world_get_particle_const(const world_t* world, int slot);

  /**
   * @brief Finds the nearest live particle within a search radius.
   *
   * @param world World to search.
   * @param position Search center in world coordinates.
   * @param radius Maximum accepted distance, in world units.
   *
   * @return The slot of the nearest particle within radius,
   *         or -1 if no particle matches or the input is invalid.
   *
   * @note The radius is inclusive: particles exactly on the boundary
   *       are accepted.
   * @note When particles have equal distances, the later examined slot
   *       can replace the earlier result.
   */
  int world_find_particle_at(const world_t* world, vec2_t position, float radius);

  /**
   * @brief Counts all live particles in a world.
   *
   * @param world World to count.
   *
   * @return Number of occupied particle slots, from 0 to max_particles.
   *         Returns 0 for a null pointer.
   *
   * @note FIXME: Complexity is O(max_particles), because the function scans
   *       the fixed array.
   */
  size_t world_particle_count(const world_t* world);

  /**
   * @brief Returns the display name for an editor tool.
   *
   * @param tool Tool identifier.
   *
   * @return A static string: "SELECT", "PROTON", "NEUTRON", or "ELECTRON".
   *         Invalid enum values return "SELECT".
   *
   * @note The returned string is owned by the program.
   */
  const char* world_tool_name(editor_tool_t tool);

  /**
   * @brief Returns the display name for a particle type.
   *
   * @param kind Particle type.
   *
   * @return A static string containing the particle name.
   *         Invalid enum values return "UNKNOWN".
   */
  const char* world_particle_name(particle_kind_t kind);

  /**
   * @brief Converts a particle kind to its save-file character.
   *
   * @param kind Particle type.
   *
   * @return 'p' for proton, 'n' for neutron, 'e' for electron,
   *         or '?' for an invalid kind.
   *
   * @note These codes are part of the atom_forge_save_v1 file format.
   */
  char world_particle_code(particle_kind_t kind);

  /**
   * @brief Writes the editable particle state to a text file.
   *
   * @param world World to serialize.
   * @param path Destination file path.
   *
   * @return true if writing and closing the file succeeded;
   *         otherwise, false.
   *
   * @note Creates or truncates the destination file.
   * @note Saves particle type and position only. It does not save velocity,
   *       selected state, camera position, zoom, simulation time,
   *       or derived chemistry.
   */
  bool world_save(const world_t* world, const char* path);

  /**
   * @brief Loads particle records from a workspace text file.
   *
   * @param world Destination world to replace after successful parsing.
   * @param path Source file path.
   *
   * @return true if the file is read and validated successfully;
   *         otherwise, false.
   *
   * @note On success, it replaces particle storage and resets derived
   *       chemistry counts, selection, tool, pause state, and
   *       simulation time.
   * @note Parsing occurs in a temporary particle array. The existing
   *       world is not replaced if validation fails.
   * @note The caller must call chemistry_rebuild() after a successful load.
   */
  bool world_load(world_t* world, const char* path);

#endif
