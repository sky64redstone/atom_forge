/*
 * Header: include/atom_forge/vec2.h
 * Implementation: src/vec2.c
 *
 * This file provides basic vector operations without depending on
 * SDL, OpenGL or the simulation. Positions, velocities, forces,
 * camera coordinates, and distances all use vec2_t.
 */

#ifndef ATOM_FORGE_VEC2_H
  #define ATOM_FORGE_VEC2_H

  #include <stdbool.h>

  typedef struct vec2 {
    float x;
    float y;
  } vec2_t;

  /**
   * @brief Constructs a two-dimensional vector.
   *
   * @param x Horizontal component of the vector.
   * @param y Vertical component of the vector.
   *
   * @return A vec2_t containing the supplied components.
   *
   * @note This function performs no validation or allocation.
   */
  vec2_t vec2(float x, float y);

  /**
   * @brief Adds two vectors component by component.
   *
   * @param a First vector.
   * @param b Second vector.
   *
   * @return The vector (a.x + b.x, a.y + b.y).
   *
   * @note Neither input vector is modified.
   */
  vec2_t vec2_add(vec2_t a, vec2_t b);

  /**
   * @brief Subtracts vector b from vector a.
   *
   * @param a Vector from which to subtract.
   * @param b Vector to subtract.
   *
   * @return The vector (a.x - b.x, a.y - b.y).
   *
   * @note Often used to obtain the displacement from a to b
   *       by calculating vec2_sub(b, a).
   */
  vec2_t vec2_sub(vec2_t a, vec2_t b);

  /**
   * @brief Multiplies a vector by a scalar.
   *
   * @param value Vector to scale.
   * @param scale Scalar multiplier.
   *
   * @return The vector (value.x * scale, value.y * scale).
   *
   * @note A negative scale reverses the vector direction.
   */
  vec2_t vec2_scale(vec2_t value, float scale);

  /**
   * @brief Reverses the direction of a vector.
   *
   * @param value Vector to negate.
   *
   * @return The vector (-value.x, -value.y).
   */
  vec2_t vec2_neg(vec2_t value);

  /**
   * @brief Calculates the dot product of two vectors.
   *
   * @param a First vector.
   * @param b Second vector.
   *
   * @return a.x * b.x + a.y * b.y.
   *
   * @note The result is a scalar, not a vector.
   */
  float vec2_dot(vec2_t a, vec2_t b);

  /**
   * @brief Calculates the squared magnitude of a vector.
   *
   * @param value Vector to measure.
   *
   * @return value.x * value.x + value.y * value.y.
   *
   * @note Avoids the square root used by vec2_len().
   *       Prefer this when comparing distances against squared thresholds.
   */
  float vec2_len2(vec2_t value);

  /**
   * @brief Calculates the magnitude of a vector.
   *
   * @param value Vector to measure.
   *
   * @return The Euclidean length sqrt(value.x^2 + value.y^2).
   */
  float vec2_len(vec2_t value);

  /**
   * @brief Calculates the squared distance between two points.
   *
   * @param a First point.
   * @param b Second point.
   *
   * @return The squared Euclidean distance between a and b.
   *
   * @note The result is always non-negative for finite inputs.
   *       No square root is calculated.
   */
  float vec2_dist2(vec2_t a, vec2_t b);

  /**
   * @brief Calculates the Euclidean distance between two points.
   *
   * @param a First point.
   * @param b Second point.
   *
   * @return The distance sqrt((a.x - b.x)^2 + (a.y - b.y)^2).
   */
  float vec2_dist(vec2_t a, vec2_t b);

  /**
   * @brief Converts a vector into a unit-length direction vector.
   *
   * @param value Vector to normalize.
   *
   * @return A vector with length approximately 1 and the same direction,
   *         or (0, 0) if the original length is at most 1.0e-6.
   *
   * @note The zero-vector fallback prevents division by a near-zero length.
   */
  vec2_t vec2_norm(vec2_t value);

  /**
   * @brief Limits a vector's magnitude without changing its direction.
   *
   * @param value Vector to limit.
   * @param max_length Maximum permitted vector magnitude.
   *
   * @return The original vector if it is already within the limit;
   *         otherwise, a scaled vector with magnitude max_length.
   *
   * @note A near-zero vector is returned unchanged.
   * @warning max_length is expected to be non-negative.
   */
  vec2_t vec2_clamp_length(vec2_t value, float max_length);

  /**
   * @brief Performs linear interpolation between two vectors.
   *
   * @param a Starting vector.
   * @param b Ending vector.
   * @param amount Interpolation factor.
   *               0 produces a; 1 produces b.
   *
   * @return a + (b - a) * amount.
   *
   * @note Values outside [0, 1] extrapolate beyond the endpoints.
   */
  vec2_t vec2_lerp(vec2_t a, vec2_t b, float amount);

  /**
   * @brief Checks whether both vector components are finite.
   *
   * @param value Vector to validate.
   *
   * @return true if both components are neither NaN nor infinity;
   *         otherwise, false.
   */
  bool vec2_is_finite(vec2_t value);

#endif
