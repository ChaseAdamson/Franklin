#pragma once
#include <vector>
#include <array>
#include <string>

// ── Data types ────────────────────────────────────────────────

struct Vec4 {
    float x, y, z, w;
};

struct Tet {
    int v[4];  // indices into vertex array
};

struct RGBA {
    float r, g, b, a;
};

struct FDRObject {
    std::string          name;
    std::vector<Vec4>    vertices;
    std::vector<Tet>     tetrahedra;
    RGBA                 rgba_global;
    std::vector<RGBA>    rgba_per_tet;  // empty if using global
    bool                 has_per_tet_color = false;
};

// ── Parser ────────────────────────────────────────────────────

// Load a .fdr file and return the parsed object.
// Throws std::runtime_error on parse failure.
FDRObject load_fdr(const std::string& filepath);
