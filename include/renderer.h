#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include "fdr_parser.h"
#include "geometry.h"

// ── Settings ──────────────────────────────────────────────────
constexpr int   WINDOW_W  = 900;
constexpr int   WINDOW_H  = 700;
constexpr int   RES       = 64;
constexpr float FOV_DEG   = 60.0f;
constexpr float HALF_LIFE = 1.0f;

// ── Light ─────────────────────────────────────────────────────
struct Light {
    glm::vec4 position;
    glm::vec3 color;     // 0-1
    float     power;
};

// ── Renderer ──────────────────────────────────────────────────
class Renderer {
public:
    Renderer();
    ~Renderer();

    void run(const FDRObject& scene_obj);

private:
    GLFWwindow* window = nullptr;

    // shader programs
    GLuint light_prog   = 0;
    GLuint compute_prog = 0;
    GLuint render_prog  = 0;

    // GPU resources
    GLuint voxel_tex  = 0;
    GLuint light_ssbo = 0;
    GLuint vao        = 0;

    // scene state
    Light  light;
    Eye4D  eye;
    Camera3D cam;
    float  angle = 0.0f;

    // render loop helpers
    void process_input(GLFWwindow* win, float dt, Eye4D& eye, Camera3D& cam);
    void draw_axes(const Camera3D& cam);
    void draw_edges(const std::vector<glm::vec4>& verts,
                    const std::vector<Tet>& tets,
                    const Eye4D& eye);
};
