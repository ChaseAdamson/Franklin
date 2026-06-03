#include "renderer.h"
#include "shaders.h"
#include "gpu_resources.h"
#include "geometry.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <cmath>
#include <cstdio>

// ── Constructor / Destructor ──────────────────────────────────

Renderer::Renderer() {
    // init GLFW
    if (!glfwInit())
        throw std::runtime_error("Failed to init GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    window = glfwCreateWindow(WINDOW_W, WINDOW_H, "FDRender", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);  // uncapped framerate

    // init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to init GLAD");

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    // compile shaders
    printf("Compiling lighting shader...\n");
    light_prog   = make_compute_program(LIGHTING_SHADER_SRC);

    printf("Compiling ray cast shader...\n");
    compute_prog = make_compute_program(COMPUTE_SHADER_SRC);

    printf("Compiling render shaders...\n");
    render_prog  = make_render_program(VERT_SHADER_SRC, FRAG_SHADER_SRC);

    // GPU resources
    voxel_tex  = make_3d_texture(RES);
    light_ssbo = make_light_ssbo(40);  // max tets

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // default light
    light.position = glm::vec4(2.0f, 2.0f, 2.0f, 2.0f);
    light.color    = glm::vec3(100.0f/255.0f, 150.0f/255.0f, 255.0f/255.0f);
    light.power    = 5000000.0f;

    // default eye
    eye.position = glm::vec4(0.0f, 2.0f, 0.0f, 0.0f);
    eye.b1       = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    eye.b2       = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    eye.b3       = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    eye.focal    = glm::vec4(0.0f, 2.0f, 0.0f, 1.0f);
}

Renderer::~Renderer() {
    glDeleteProgram(light_prog);
    glDeleteProgram(compute_prog);
    glDeleteProgram(render_prog);
    glDeleteTextures(1, &voxel_tex);
    glDeleteBuffers(1, &light_ssbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}

// ── Input processing ──────────────────────────────────────────

void Renderer::process_input(GLFWwindow* win, float dt, Eye4D& e, Camera3D& c) {
    constexpr float MOVE  = 1.0f;
    constexpr float ROT   = 1.0f;
    constexpr float ORBIT = 60.0f;

    // 3D viewer orbit
    if (glfwGetKey(win, GLFW_KEY_LEFT)  == GLFW_PRESS) c.yaw   -= ORBIT * dt;
    if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) c.yaw   += ORBIT * dt;
    if (glfwGetKey(win, GLFW_KEY_UP)    == GLFW_PRESS) c.pitch -= ORBIT * dt;
    if (glfwGetKey(win, GLFW_KEY_DOWN)  == GLFW_PRESS) c.pitch += ORBIT * dt;
    if (glfwGetKey(win, GLFW_KEY_Q)     == GLFW_PRESS) c.dist = std::max(1.5f, c.dist - 2.0f * dt);
    if (glfwGetKey(win, GLFW_KEY_E)     == GLFW_PRESS) c.dist = std::min(20.0f, c.dist + 2.0f * dt);
    c.pitch = std::max(-89.0f, std::min(89.0f, c.pitch));

    // 4D eye translation
    glm::vec4 look_dir = e.focal - e.position;
    float look_len = glm::length(look_dir);
    glm::vec4 look_hat = (look_len > 1e-10f)
        ? look_dir / look_len
        : glm::vec4(0,0,0,1);

    glm::vec4 move(0.0f);
    if (glfwGetKey(win, GLFW_KEY_I) == GLFW_PRESS) move += look_hat;
    if (glfwGetKey(win, GLFW_KEY_K) == GLFW_PRESS) move -= look_hat;
    if (glfwGetKey(win, GLFW_KEY_L) == GLFW_PRESS) move += e.b1;
    if (glfwGetKey(win, GLFW_KEY_J) == GLFW_PRESS) move -= e.b1;
    if (glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS) move += e.b3;
    if (glfwGetKey(win, GLFW_KEY_U) == GLFW_PRESS) move -= e.b3;
    if (glm::length(move) > 1e-10f)
        translate_eye(e, move * MOVE * dt);

    // 4D eye rotation
    if (glfwGetKey(win, GLFW_KEY_T) == GLFW_PRESS) apply_rotation(e, rot_xz( ROT * dt));
    if (glfwGetKey(win, GLFW_KEY_Y) == GLFW_PRESS) apply_rotation(e, rot_xz(-ROT * dt));
    if (glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS) apply_rotation(e, rot_xw( ROT * dt));
    if (glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS) apply_rotation(e, rot_xw(-ROT * dt));
    if (glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS) apply_rotation(e, rot_zw( ROT * dt));
    if (glfwGetKey(win, GLFW_KEY_N) == GLFW_PRESS) apply_rotation(e, rot_zw(-ROT * dt));
}

// ── Axis overlay ──────────────────────────────────────────────

void Renderer::draw_axes(const Camera3D& c) {
    glm::vec3 pos = c.position();
    glm::vec3 fwd = c.forward();
    glm::vec3 up  = c.up();

    glm::mat4 proj = glm::perspective(
        glm::radians(FOV_DEG),
        (float)WINDOW_W / (float)WINDOW_H,
        0.1f, 100.0f
    );
    glm::mat4 view = glm::lookAt(pos, pos + fwd, up);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));

    float end = 2.0f;
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(1.5,0,0); glVertex3f(1,0,0); glVertex3f(end,0,0);
    glColor3f(0,1.5,0); glVertex3f(0,1,0); glVertex3f(0,end,0);
    glColor3f(0,0,1.5); glVertex3f(0,0,1); glVertex3f(0,0,end);
    glEnd();
}

// ── Edge overlay ──────────────────────────────────────────────

void Renderer::draw_edges(
    const std::vector<glm::vec4>& verts,
    const std::vector<Tet>& tets,
    const Eye4D& e)
{
    glm::vec4 look     = e.focal - e.position;
    float     look_len = glm::length(look);
    glm::vec4 look_hat = (look_len > 1e-10f) ? look / look_len : glm::vec4(0,0,0,1);
    glm::vec4 focal    = e.focal;

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.9f, 0.5f);

    for (const auto& tet : tets) {
        glm::vec3 proj[4];
        bool      valid[4] = {false, false, false, false};

        for (int j = 0; j < 4; j++) {
            glm::vec4 v       = verts[tet.v[j]];
            glm::vec4 ray_dir = v - focal;
            float denom = glm::dot(look_hat, ray_dir);
            if (fabsf(denom) < 1e-10f) continue;
            float t = glm::dot(look_hat, e.position - focal) / denom;
            glm::vec4 rp  = focal + t * ray_dir;
            glm::vec4 rel = rp - e.position;
            float fi = glm::dot(rel, e.b1);
            float fj = glm::dot(rel, e.b2);
            float fk = glm::dot(rel, e.b3);
            proj[j]  = glm::vec3(-fi, -fj, -fk);
            valid[j] = true;
        }

        int pairs[6][2] = {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
        for (auto& p : pairs) {
            if (!valid[p[0]] || !valid[p[1]]) continue;
            glVertex3f(proj[p[0]].x, proj[p[0]].y, proj[p[0]].z);
            glVertex3f(proj[p[1]].x, proj[p[1]].y, proj[p[1]].z);
        }
    }
    glEnd();
}

// ── Main run loop ─────────────────────────────────────────────

void Renderer::run(const FDRObject& obj) {
    float aspect    = (float)WINDOW_W / WINDOW_H;
    float fov_scale = tanf(glm::radians(FOV_DEG * 0.5f));
    float voxel_sz  = 2.0f / RES;
    float step_size = voxel_sz * 0.5f;
    float extinction = logf(2.0f) / HALF_LIFE;

    glm::vec4 offset(0.0f, 2.0f, 0.0f, 4.0f);  // TESSERACT_OFFSET

    glm::vec3 obj_color(
        obj.rgba_global.r,
        obj.rgba_global.g,
        obj.rgba_global.b
    );

    double prev_time = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float  dt  = (float)(now - prev_time);
        prev_time  = now;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        process_input(window, dt, eye, cam);

        // rotate geometry (frozen — set angle += ... to enable)
        glm::mat4 R = rot_xw_legacy(angle);
        std::vector<glm::vec4> rotated = apply_transform(obj.vertices, R, offset);

        // ── lighting pass ─────────────────────────────────────
        dispatch_lighting(light_prog, light_ssbo,
                          rotated, obj.tetrahedra,
                          light.position, light.color, light.power,
                          obj_color);

        // ── ray cast pass ─────────────────────────────────────
        dispatch_raycast(compute_prog, voxel_tex, light_ssbo,
                         rotated, obj.tetrahedra,
                         eye, RES);

        // ── render pass ───────────────────────────────────────
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(render_prog);
        upload_render_uniforms(render_prog, cam, aspect, fov_scale,
                               extinction, step_size);
        glUseProgram(render_prog);  // re-bind after upload_render_uniforms unbinds it

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, voxel_tex);
        glUniform1i(glGetUniformLocation(render_prog, "voxel_tex"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glUseProgram(0);

        // ── overlays ──────────────────────────────────────────
        draw_axes(cam);
        draw_edges(rotated, obj.tetrahedra, eye);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
