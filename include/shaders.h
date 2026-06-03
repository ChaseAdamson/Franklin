#pragma once
#include <string>
#include <glad/glad.h>

// ── Shader source strings ─────────────────────────────────────
extern const char* LIGHTING_SHADER_SRC;
extern const char* COMPUTE_SHADER_SRC;
extern const char* VERT_SHADER_SRC;
extern const char* FRAG_SHADER_SRC;

// ── Compilation helpers ───────────────────────────────────────

// Compile a single shader stage. Throws on error.
GLuint compile_shader(const char* src, GLenum type);

// Link a compute shader program. Throws on error.
GLuint make_compute_program(const char* src);

// Link a vertex + fragment shader program. Throws on error.
GLuint make_render_program(const char* vert_src, const char* frag_src);
