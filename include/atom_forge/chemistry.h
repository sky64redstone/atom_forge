/*
 * Header: include/atom_forge/chemistry.h
 * Implementation: src/chemistry.c
 *
 * The chemistry module derives atoms and molecules from the particles stored
 * in world_t. It does not own a separate permanent particle model.
 * Its main entry point is chemistry_rebuild().
 */

#ifndef ATOM_FORGE_CHEMISTRY_H
  #define ATOM_FORGE_CHEMISTRY_H

  #include <stdbool.h>

  #include "atom_forge/world.h"

  /**
   * @brief Looks up an element name using its proton count.
   *
   * @param proton_count Number of protons; interpreted as the atomic number Z.
   *
   * @return Pointer to a static lowercase element name for Z in [1, 20].
   *         Returns "unknown" if Z is zero or greater than 20.
   *
   * @note This lookup identifies the element, not the isotope.
   */
  const char* chemistry_element_name(unsigned int proton_count);

  /**
   * @brief Looks up an element symbol using its proton count.
   *
   * @param proton_count Number of protons (atomic number Z).
   *
   * @return Pointer to the static element symbol, such as "H", "He",
   *         "C", or "Ca". Returns "?" when Z is unsupported.
   */
  const char* chemistry_element_symbol(unsigned int proton_count);

  /**
   * @brief Tests whether a proton/neutron composition appears in the
   *        project's selected stable-isotope table.
   *
   * @param proton_count Number of protons.
   * @param neutron_count Number of neutrons.
   *
   * @return true if the exact pair is listed in stable_isotopes[];
   *         false otherwise.
   *
   * @warning false means "not listed as stable by this implementation,"
   *          not necessarily "scientifically confirmed radioactive."
   */
  bool chemistry_isotope_stable(
    unsigned int proton_count,
    unsigned int neutron_count
  );

  /**
   * @brief Returns the prototype's nominal valence capacity for a neutral
   *        atom of the specified element.
   *
   * @param proton_count Number of protons (atomic number Z).
   *
   * @return The table value for elements 1–20, or 0 when Z is unsupported
   *         or the element has a configured valence of zero.
   *
   * @note This is a simplified integer rule, not a full treatment
   *       of electronic structure.
   */
  int chemistry_neutral_valence(unsigned int proton_count);

  /**
   * @brief Calculates the prototype's bond capacity for an atom with a
   *        given proton and electron count.
   *
   * @param proton_count Number of protons.
   * @param electron_count Number of electrons assigned to the atom.
   *
   * @return An integer capacity from 0 to 4.
   *
   * @note The result is derived from the nominal neutral valence and a
   *       simple net-charge rule. It is not a complete chemical model.
   */
  int chemistry_valence_capacity(
    unsigned int proton_count,
    unsigned int electron_count
  );

  /**
   * @brief Recalculates nuclei, element identities, electron assignments,
   *        bond connectivity, and molecules from the current particles.
   *
   * @param world World to recalculate.
   *
   * @return Nothing.
   *
   * @note Clears and rebuilds world->atoms, world->bonds and world->molecules,
   *       updating counts, atom positions, charges, valence capacities,
   *       labels, and molecule IDs.
   * @note Particle positions and types are read, not changed.
   * @note Atom and molecule array indices may change on every rebuild.
   * @note A null world pointer is ignored.
   */
  void chemistry_rebuild(world_t* world);

#endif
