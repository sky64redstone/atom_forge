#include <glad/gl.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>

#include "atom_forge/renderer.h"
#include "renderer_internal.h"

static const uint8_t font_rows[128][7] = {
  ['!'] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 },
  ['+'] = { 0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00 },
  ['-'] = { 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00 },
  ['/'] = { 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10 },
  [':'] = { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 },
  ['|'] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
  ['.'] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c },
  ['?'] = { 0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },
  ['='] = { 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00 },
  ['0'] = { 0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e },
  ['1'] = { 0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e },
  ['2'] = { 0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f },
  ['3'] = { 0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e },
  ['4'] = { 0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02 },
  ['5'] = { 0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e },
  ['6'] = { 0x0e, 0x10, 0x10, 0x1e, 0x11, 0x11, 0x0e },
  ['7'] = { 0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },
  ['8'] = { 0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e },
  ['9'] = { 0x0e, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x0e },
  ['A'] = { 0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11 },
  ['B'] = { 0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e },
  ['C'] = { 0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e },
  ['D'] = { 0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e },
  ['E'] = { 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f },
  ['F'] = { 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10 },
  ['G'] = { 0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f },
  ['H'] = { 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11 },
  ['I'] = { 0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e },
  ['J'] = { 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0c },
  ['K'] = { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 },
  ['L'] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f },
  ['M'] = { 0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11 },
  ['N'] = { 0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11 },
  ['O'] = { 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e },
  ['P'] = { 0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10 },
  ['Q'] = { 0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d },
  ['R'] = { 0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11 },
  ['S'] = { 0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e },
  ['T'] = { 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
  ['U'] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e },
  ['V'] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04 },
  ['W'] = { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a },
  ['X'] = { 0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11 },
  ['Y'] = { 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04 },
  ['Z'] = { 0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f }
};


static void renderer_append_vertex(
  renderer_t* renderer,
  float x,
  float y,
  color4_t color
) {
  if (renderer->overlay_vertex_count >= renderer->overlay_vertex_capacity) {
    renderer_flush_overlay(renderer);
  }
  size_t offset = renderer->overlay_vertex_count * 6;
  renderer->overlay_vertices[offset + 0] = x;
  renderer->overlay_vertices[offset + 1] = y;
  renderer->overlay_vertices[offset + 2] = color.r;
  renderer->overlay_vertices[offset + 3] = color.g;
  renderer->overlay_vertices[offset + 4] = color.b;
  renderer->overlay_vertices[offset + 5] = color.a;
  ++renderer->overlay_vertex_count;
}

static void renderer_append_triangle(
  renderer_t* renderer,
  float x0,
  float y0,
  float x1,
  float y1,
  float x2,
  float y2,
  color4_t color
) {
  renderer_append_vertex(renderer, x0, y0, color);
  renderer_append_vertex(renderer, x1, y1, color);
  renderer_append_vertex(renderer, x2, y2, color);
}


void renderer_draw_line(
  renderer_t* renderer,
  float x0,
  float y0,
  float x1,
  float y1,
  float thickness,
  color4_t color
) {
  if (renderer == NULL) {
    return;
  }
  float dx = x1 - x0;
  float dy = y1 - y0;
  float length = sqrtf(dx * dx + dy * dy);
  if (length <= 1.0e-4f) {
    return;
  }
  float nx = -dy / length * thickness * 0.5f;
  float ny = dx / length * thickness * 0.5f;
  float ax = x0 + nx;
  float ay = y0 + ny;
  float bx = x1 + nx;
  float by = y1 + ny;
  float cx = x1 - nx;
  float cy = y1 - ny;
  float dx2 = x0 - nx;
  float dy2 = y0 - ny;
  renderer_append_triangle(renderer, ax, ay, bx, by, cx, cy, color);
  renderer_append_triangle(renderer, ax, ay, cx, cy, dx2, dy2, color);
}

void renderer_draw_rect(
  renderer_t* renderer,
  float x,
  float y,
  float width,
  float height,
  color4_t color
) {
  if (renderer == NULL || width <= 0.0f || height <= 0.0f) {
    return;
  }
  float right = x + width;
  float bottom = y + height;
  renderer_append_triangle(renderer, x, y, right, y, right, bottom, color);
  renderer_append_triangle(renderer, x, y, right, bottom, x, bottom, color);
}

void renderer_draw_text(
  renderer_t* renderer,
  float x,
  float y,
  const char* text,
  int scale,
  color4_t color
) {
  if (renderer == NULL || text == NULL || scale < 1) {
    return;
  }
  float cursor_x = x;
  for (const unsigned char* current = (const unsigned char*)text; *current != '\0'; ++current) {
    unsigned char character = *current;
    if (character >= 'a' && character <= 'z') {
      character = (unsigned char)(character - 'a' + 'A');
    }
    if (character >= 128) {
      character = '?';
    }
    const uint8_t* rows = font_rows[character];
    if (character != ' ' && rows[0] == 0 && rows[1] == 0 && rows[2] == 0 &&
        rows[3] == 0 && rows[4] == 0 && rows[5] == 0 && rows[6] == 0) {
      rows = font_rows[(unsigned char)'?'];
    }
    for (int row = 0; row < 7; ++row) {
      for (int column = 0; column < 5; ++column) {
        if ((rows[row] & (uint8_t)(1u << (4 - column))) != 0) {
          renderer_draw_rect(
            renderer,
            cursor_x + (float)(column * scale),
            y + (float)(row * scale),
            (float)scale,
            (float)scale,
            color
          );
        }
      }
    }
    cursor_x += (float)(6 * scale);
  }
}

void renderer_flush_overlay(renderer_t* renderer) {
  if (renderer == NULL || !renderer->initialized || renderer->overlay_vertex_count == 0) {
    return;
  }
  glUseProgram(renderer->overlay_program);
  glUniform2f(
    renderer->overlay_resolution_location,
    renderer->width,
    renderer->height
  );
  glBindVertexArray(renderer->overlay_vao);
  glBindBuffer(GL_ARRAY_BUFFER, renderer->overlay_vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizeiptr)(renderer->overlay_vertex_count * 6 * sizeof(float)),
    renderer->overlay_vertices,
    GL_STREAM_DRAW
  );
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)renderer->overlay_vertex_count);
  glBindVertexArray(0);
  renderer->overlay_vertex_count = 0;
}


void renderer_present(renderer_t* renderer) {
  renderer_flush_overlay(renderer);
}
