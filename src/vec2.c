#include <math.h>

#include "atom_forge/vec2.h"

vec2_t vec2(float x, float y) {
  return (vec2_t){ .x = x, .y = y };
}

vec2_t vec2_add(vec2_t a, vec2_t b) {
  return vec2(a.x + b.x, a.y + b.y);
}

vec2_t vec2_sub(vec2_t a, vec2_t b) {
  return vec2(a.x - b.x, a.y - b.y);
}

vec2_t vec2_scale(vec2_t value, float scale) {
  return vec2(value.x * scale, value.y * scale);
}

vec2_t vec2_neg(vec2_t value) {
  return vec2(-value.x, -value.y);
}

float vec2_dot(vec2_t a, vec2_t b) {
  return a.x * b.x + a.y * b.y;
}

float vec2_len2(vec2_t value) {
  return vec2_dot(value, value);
}

float vec2_len(vec2_t value) {
  return sqrtf(vec2_len2(value));
}

float vec2_dist2(vec2_t a, vec2_t b) {
  return vec2_len2(vec2_sub(a, b));
}

float vec2_dist(vec2_t a, vec2_t b) {
  return sqrtf(vec2_dist2(a, b));
}

vec2_t vec2_norm(vec2_t value) {
  float length = vec2_len(value);
  /* Return (0, 0) for near-zero vectors instead of dividing by zero. */
  if (length <= 1.0e-6f) {
    return vec2(0.0f, 0.0f);
  }
  return vec2_scale(value, 1.0f / length);
}

/* Preserve direction while limiting the vector magnitude. */
vec2_t vec2_clamp_length(vec2_t value, float max_length) {
  float length_sq = vec2_len2(value);
  float max_sq = max_length * max_length;
  if (length_sq <= max_sq || length_sq <= 1.0e-6f) {
    return value;
  }
  return vec2_scale(value, max_length / sqrtf(length_sq));
}

vec2_t vec2_lerp(vec2_t a, vec2_t b, float amount) {
  return vec2_add(a, vec2_scale(vec2_sub(b, a), amount));
}

bool vec2_is_finite(vec2_t value) {
  return isfinite(value.x) && isfinite(value.y);
}
