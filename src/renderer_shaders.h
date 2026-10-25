#ifndef ATOM_FORGE_RENDERER_SHADERS_H
  #define ATOM_FORGE_RENDERER_SHADERS_H

  static const char* renderer_grid_vertex_shader =
    "#version 330 core\n"
    "void main() {\n"
    "  vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
    "  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n"
    "}\n";

  static const char* renderer_grid_fragment_shader =
    "#version 330 core\n"
    "out vec4 out_color;\n"
    "uniform vec2 u_resolution;\n"
    "uniform vec2 u_camera;\n"
    "uniform float u_zoom;\n"
    "uniform float u_time;\n"
    "float grid_line(vec2 world, float spacing, float width_px) {\n"
    "  vec2 d = abs(fract(world / spacing + 0.5) - 0.5) * spacing * u_zoom;\n"
    "  float distance_px = min(d.x, d.y);\n"
    "  return 1.0 - smoothstep(width_px, width_px + 0.8, distance_px);\n"
    "}\n"
    "void main() {\n"
    "  vec2 world = u_camera + (gl_FragCoord.xy - u_resolution * 0.5) / u_zoom;\n"
    "  float minor_grid = grid_line(world, 1.0, 0.35);\n"
    "  float major_grid = grid_line(world, 5.0, 0.65);\n"
    "  float axis_x = 1.0 - smoothstep(0.35, 1.0, abs(world.x * u_zoom));\n"
    "  float axis_y = 1.0 - smoothstep(0.35, 1.0, abs(world.y * u_zoom));\n"
    "  vec3 color = vec3(0.025, 0.034, 0.050);\n"
    "  color = mix(color, vec3(0.070, 0.088, 0.112), minor_grid * 0.72);\n"
    "  color = mix(color, vec3(0.105, 0.135, 0.165), major_grid * 0.78);\n"
    "  color = mix(color, vec3(0.17, 0.26, 0.31), max(axis_x, axis_y) * 0.55);\n"
    "  float vignette = 1.0 - smoothstep(0.12, 1.35,\n"
    "    length((gl_FragCoord.xy / u_resolution - 0.5) * vec2(1.0, 0.85)));\n"
    "  color *= 0.82 + 0.18 * vignette;\n"
    "  out_color = vec4(color, 1.0);\n"
    "}\n";

  static const char* renderer_particle_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_quad;\n"
    "layout(location = 1) in vec2 a_center;\n"
    "layout(location = 2) in float a_radius;\n"
    "layout(location = 3) in vec4 a_color;\n"
    "layout(location = 4) in float a_mode;\n"
    "uniform vec2 u_resolution;\n"
    "uniform vec2 u_camera;\n"
    "uniform float u_zoom;\n"
    "out vec2 v_local;\n"
    "out vec4 v_color;\n"
    "out float v_mode;\n"
    "void main() {\n"
    "  float radius_px = clamp(a_radius * u_zoom, 3.2, 15.0);\n"
    "  vec2 pixel = (a_center - u_camera) * u_zoom + u_resolution * 0.5;\n"
    "  pixel += a_quad * radius_px;\n"
    "  vec2 clip = pixel / u_resolution * 2.0 - 1.0;\n"
    "  gl_Position = vec4(clip, 0.0, 1.0);\n"
    "  v_local = a_quad;\n"
    "  v_color = a_color;\n"
    "  v_mode = a_mode;\n"
    "}\n";

  static const char* renderer_particle_fragment_shader =
    "#version 330 core\n"
    "in vec2 v_local;\n"
    "in vec4 v_color;\n"
    "in float v_mode;\n"
    "uniform float u_time;\n"
    "out vec4 out_color;\n"
    "void main() {\n"
    "  float d = length(v_local);\n"
    "  if (d > 1.0) discard;\n"
    "  if (v_mode < 0.5) {\n"
    "    float alpha = 1.0 - smoothstep(0.91, 1.0, d);\n"
    "    float highlight = (1.0 - smoothstep(0.0, 0.95, d)) * 0.22;\n"
    "    vec3 color = mix(v_color.rgb, vec3(1.0), highlight);\n"
    "    out_color = vec4(color, alpha * v_color.a);\n"
    "  } else if (v_mode < 1.5) {\n"
    "    float ring = smoothstep(0.78, 0.87, d) * (1.0 - smoothstep(0.94, 1.0, d));\n"
    "    out_color = vec4(v_color.rgb, ring * v_color.a);\n"
    "  } else {\n"
    "    float pulse = 0.72 + 0.28 * sin(u_time * 5.0);\n"
    "    float halo = exp(-abs(d - 0.78) * 18.0) * pulse;\n"
    "    float ring = smoothstep(0.67, 0.82, d) * (1.0 - smoothstep(0.94, 1.0, d));\n"
    "    out_color = vec4(v_color.rgb, max(halo * 0.46, ring * 0.8) * v_color.a);\n"
    "  }\n"
    "}\n";

  static const char* renderer_overlay_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_position;\n"
    "layout(location = 1) in vec4 a_color;\n"
    "uniform vec2 u_resolution;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "  vec2 clip = vec2(\n"
    "    a_position.x / u_resolution.x * 2.0 - 1.0,\n"
    "    1.0 - a_position.y / u_resolution.y * 2.0\n"
    "  );\n"
    "  gl_Position = vec4(clip, 0.0, 1.0);\n"
    "  v_color = a_color;\n"
    "}\n";

  static const char* renderer_overlay_fragment_shader =
    "#version 330 core\n"
    "in vec4 v_color;\n"
    "out vec4 out_color;\n"
    "void main() { out_color = v_color; }\n";


#endif
