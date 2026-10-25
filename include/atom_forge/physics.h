/*
 * Header: include/atom_forge/physics.h
 * Implementation: src/physics.c
 *
 * The physics module updates particle motion and attempts simplified
 * radioactive-decay events. It operates directly on the editable particle
 * slots and invokes chemistry_rebuild() to recalculate derived atoms after
 * particles have moved or changed type.
 */

#ifndef ATOM_FORGE_PHYSICS_H
  #define ATOM_FORGE_PHYSICS_H

  #include "atom_forge/world.h"

  /**
   * @brief Advances a running world by one simulation time step.
   *
   * @param world World to simulate.
   * @param delta_time Requested elapsed time in seconds.
   *
   * @return Nothing.
   *
   * @sideeffect If the world is running, advances simulation time,
   *             updates particle motion, rebuilds chemistry, increments
   *             radioactive timers, and may trigger decay events.
   *
   * @note Clamps delta_time to [0, 0.05] seconds.
   * @note A null or paused world is not advanced.
   */
  void physics_update(world_t* world, float delta_time);

#endif
