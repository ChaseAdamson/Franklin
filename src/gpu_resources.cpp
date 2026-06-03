#include "gpu_resources.h"
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <cmath>

// ── Helper: get uniform location ─────────────────────────────
static GLint loc(GLuint prog, const char* name) {
    return glGetUniformLocation(prog, name);
}

// ── Texture ───────────────────────────────────────────────────

GLuint make_3d_texture(int res) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8,
                 res, res, res, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_3D, 0);
    return tex;
}

// ── SSBO ──────────────────────────────────────────────────────

GLuint make_light_ssbo(int n_tets) {
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 n_tets * 4 * sizeof(glm::vec4),
                 nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

// ── Lighting uniforms ─────────────────────────────────────────

void upload_lighting_uniforms(
    GLuint program,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const glm::vec4& light_pos,
    const glm::vec3& light_color,
    float light_power,
    const glm::vec3& obj_color)
{
    glUseProgram(program);

    int n_tets  = (int)tetrahedra.size();
    int n_verts = (int)vertices.size();
    glUniform1i(loc(program, "n_tets"),  n_tets);
    glUniform1i(loc(program, "n_verts"), n_verts);

    for (int i = 0; i < n_verts; i++) {
        std::string name = "verts[" + std::to_string(i) + "]";
        glUniform4fv(loc(program, name.c_str()), 1, glm::value_ptr(vertices[i]));
    }
    for (int i = 0; i < n_tets; i++) {
        std::string name = "tet_idx[" + std::to_string(i) + "]";
        glUniform4i(loc(program, name.c_str()),
                    tetrahedra[i].v[0], tetrahedra[i].v[1],
                    tetrahedra[i].v[2], tetrahedra[i].v[3]);
    }

    glUniform4fv(loc(program, "light_pos"),   1, glm::value_ptr(light_pos));
    glUniform3fv(loc(program, "light_color"), 1, glm::value_ptr(light_color));
    glUniform1f (loc(program, "light_power"), light_power);
    glUniform3fv(loc(program, "obj_color"),   1, glm::value_ptr(obj_color));

    glUseProgram(0);
}

// ── Ray cast uniforms ─────────────────────────────────────────

void upload_raycast_uniforms(
    GLuint program,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const Eye4D& eye,
    int res)
{
    glUseProgram(program);

    int n_tets = (int)tetrahedra.size();
    glUniform1i(loc(program, "n_tets"), n_tets);
    glUniform1i(loc(program, "res"),    res);

    // eye
    glm::vec4 look = eye.focal - eye.position;
    glUniform4fv(loc(program, "eye_pos"),  1, glm::value_ptr(eye.position));
    glUniform4fv(loc(program, "eye_look"), 1, glm::value_ptr(look));
    glUniform4fv(loc(program, "eye_b1"),   1, glm::value_ptr(eye.b1));
    glUniform4fv(loc(program, "eye_b2"),   1, glm::value_ptr(eye.b2));
    glUniform4fv(loc(program, "eye_b3"),   1, glm::value_ptr(eye.b3));

    // tet_v and tet_n — computed from vertices
    for (int i = 0; i < n_tets; i++) {
        for (int j = 0; j < 4; j++) {
            std::string name = "tet_v[" + std::to_string(i) + "][" + std::to_string(j) + "]";
            int vi = tetrahedra[i].v[j];
            glUniform4fv(loc(program, name.c_str()), 1, glm::value_ptr(vertices[vi]));
        }
        // compute normal
        glm::vec4 v0 = vertices[tetrahedra[i].v[0]];
        glm::vec4 v1 = vertices[tetrahedra[i].v[1]];
        glm::vec4 v2 = vertices[tetrahedra[i].v[2]];
        glm::vec4 v3 = vertices[tetrahedra[i].v[3]];
        // 4D cross product
        glm::vec4 a = v1-v0, b = v2-v0, c = v3-v0;
        glm::vec4 n(
            +(a.y*(b.z*c.w-b.w*c.z) - a.z*(b.y*c.w-b.w*c.y) + a.w*(b.y*c.z-b.z*c.y)),
            -(a.x*(b.z*c.w-b.w*c.z) - a.z*(b.x*c.w-b.w*c.x) + a.w*(b.x*c.z-b.z*c.x)),
            +(a.x*(b.y*c.w-b.w*c.y) - a.y*(b.x*c.w-b.w*c.x) + a.w*(b.x*c.y-b.y*c.x)),
            -(a.x*(b.y*c.z-b.z*c.y) - a.y*(b.x*c.z-b.z*c.x) + a.z*(b.x*c.y-b.y*c.x))
        );
        float n_len = glm::length(n);
        if (n_len > 1e-10f) n /= n_len;
        std::string nname = "tet_n[" + std::to_string(i) + "]";
        glUniform4fv(loc(program, nname.c_str()), 1, glm::value_ptr(n));
    }

    glUseProgram(0);
}

// ── Render uniforms ───────────────────────────────────────────

void upload_render_uniforms(
    GLuint program,
    const Camera3D& cam,
    float aspect,
    float fov_scale,
    float extinction,
    float step_size)
{
    glUseProgram(program);

    glm::vec3 pos = cam.position();
    glm::vec3 fwd = cam.forward();
    glm::vec3 rgt = cam.right();
    glm::vec3 up  = cam.up();

    glUniform3fv(loc(program, "cam_pos"),     1, glm::value_ptr(pos));
    glUniform3fv(loc(program, "cam_forward"), 1, glm::value_ptr(fwd));
    glUniform3fv(loc(program, "cam_right"),   1, glm::value_ptr(rgt));
    glUniform3fv(loc(program, "cam_up"),      1, glm::value_ptr(up));
    glUniform1f (loc(program, "aspect"),      aspect);
    glUniform1f (loc(program, "fov_scale"),   fov_scale);
    glUniform1f (loc(program, "extinction"),  extinction);
    glUniform1f (loc(program, "step_size"),   step_size);

    glUseProgram(0);
}

// ── Dispatch ──────────────────────────────────────────────────

void dispatch_lighting(
    GLuint program,
    GLuint ssbo,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const glm::vec4& light_pos,
    const glm::vec3& light_color,
    float light_power,
    const glm::vec3& obj_color)
{
    upload_lighting_uniforms(program, vertices, tetrahedra,
                             light_pos, light_color, light_power, obj_color);
    glUseProgram(program);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);
    glDispatchCompute((GLuint)tetrahedra.size(), 4, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glUseProgram(0);
}

void dispatch_raycast(
    GLuint program,
    GLuint texture,
    GLuint ssbo,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const Eye4D& eye,
    int res)
{
    upload_raycast_uniforms(program, vertices, tetrahedra, eye, res);
    glUseProgram(program);
    glBindImageTexture(0, texture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo);  // already bound; just ensure visible
    int gx = (res + 7) / 8;
    int gy = (res + 7) / 8;
    int gz = (res + 3) / 4;
    glDispatchCompute(gx, gy, gz);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glUseProgram(0);
}
