#include "shaders.h"
#include <stdexcept>
#include <string>

// ── Lighting compute shader ───────────────────────────────────
// One invocation per vertex per tet (dispatch N_TETS x 4 x 1).
// Writes per-vertex lit RGB into an SSBO at binding=2.

const char* LIGHTING_SHADER_SRC = R"(
#version 430

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 2) buffer LightBuffer {
    vec4 vert_colors[];
};

uniform vec4  verts[16];
uniform ivec4 tet_idx[40];
uniform int   n_tets;
uniform int   n_verts;

uniform vec4  light_pos;
uniform vec3  light_color;
uniform float light_power;
uniform vec3  obj_color;

vec4 cross4(vec4 a, vec4 b, vec4 c) {
    return vec4(
        +( a.y*(b.z*c.w-b.w*c.z) - a.z*(b.y*c.w-b.w*c.y) + a.w*(b.y*c.z-b.z*c.y)),
        -( a.x*(b.z*c.w-b.w*c.z) - a.z*(b.x*c.w-b.w*c.x) + a.w*(b.x*c.z-b.z*c.x)),
        +( a.x*(b.y*c.w-b.w*c.y) - a.y*(b.x*c.w-b.w*c.x) + a.w*(b.x*c.y-b.y*c.x)),
        -( a.x*(b.y*c.z-b.z*c.y) - a.y*(b.x*c.z-b.z*c.x) + a.z*(b.x*c.y-b.y*c.x))
    );
}

bool barycentric_shadow(vec4 p, vec4 v0, vec4 v1, vec4 v2, vec4 v3) {
    vec4 e1=v1-v0; vec4 e2=v2-v0; vec4 e3=v3-v0; vec4 b=p-v0;
    float a00=dot(e1,e1); float a01=dot(e1,e2); float a02=dot(e1,e3);
    float a11=dot(e2,e2); float a12=dot(e2,e3); float a22=dot(e3,e3);
    float r0=dot(e1,b); float r1=dot(e2,b); float r2=dot(e3,b);
    float det=a00*(a11*a22-a12*a12)-a01*(a01*a22-a12*a02)+a02*(a01*a12-a11*a02);
    if (abs(det)<1e-10) return false;
    float s=(r0*(a11*a22-a12*a12)-a01*(r1*a22-a12*r2)+a02*(r1*a12-a11*r2))/det;
    float t=(a00*(r1*a22-a12*r2)-r0*(a01*a22-a12*a02)+a02*(a01*r2-r1*a02))/det;
    float u=(a00*(a11*r2-r1*a12)-a01*(a01*r2-r1*a02)+r0*(a01*a12-a11*a02))/det;
    vec4 residual=b-s*e1-t*e2-u*e3;
    if (dot(residual,residual)>1e-8) return false;
    float eps=1e-4;
    return s>=-eps && t>=-eps && u>=-eps && (s+t+u)<=1.0+eps;
}

void main() {
    int tet_id  = int(gl_GlobalInvocationID.x);
    int vert_id = int(gl_GlobalInvocationID.y);
    if (tet_id >= n_tets || vert_id >= 4) return;

    vec4 v0 = verts[tet_idx[tet_id].x];
    vec4 v1 = verts[tet_idx[tet_id].y];
    vec4 v2 = verts[tet_idx[tet_id].z];
    vec4 v3 = verts[tet_idx[tet_id].w];
    vec4 n   = cross4(v1-v0, v2-v0, v3-v0);
    float n_len = length(n);
    vec4 n_hat = (n_len > 1e-10) ? n/n_len : vec4(0.0);

    vec4 vpos;
    if      (vert_id == 0) vpos = v0;
    else if (vert_id == 1) vpos = v1;
    else if (vert_id == 2) vpos = v2;
    else                   vpos = v3;

    vec4  to_light  = light_pos - vpos;
    float r         = length(to_light);
    vec3  out_color = vec3(0.0);

    if (r > 1e-6) {
        vec4 shadow_dir = to_light / r;
        bool in_shadow  = false;

        for (int j = 0; j < n_tets; j++) {
            vec4 sv0=verts[tet_idx[j].x];
            vec4 sv1=verts[tet_idx[j].y];
            vec4 sv2=verts[tet_idx[j].z];
            vec4 sv3=verts[tet_idx[j].w];
            vec4 sn=cross4(sv1-sv0,sv2-sv0,sv3-sv0);
            float sn_len=length(sn);
            if (sn_len<1e-10) continue;
            vec4 sn_hat=sn/sn_len;
            float sdenom=dot(sn_hat,shadow_dir);
            if (abs(sdenom)<1e-10) continue;
            float st=(dot(sn_hat,sv0)-dot(sn_hat,vpos))/sdenom;
            if (st<=1e-4 || st>=r-1e-4) continue;
            if (barycentric_shadow(vpos+st*shadow_dir,sv0,sv1,sv2,sv3)) {
                in_shadow=true;
                break;
            }
        }

        if (!in_shadow) {
            float cosine = abs(dot(shadow_dir, n_hat));
            vec3 raw = obj_color * light_color * light_power * cosine / (r*r*r);
            out_color = clamp(raw, 0.0, 1.0);
        }
    }

    vert_colors[tet_id * 4 + vert_id] = vec4(out_color, 1.0);
}
)";

// ── Ray cast compute shader ───────────────────────────────────
// One invocation per voxel (dispatch res/8 x res/8 x res/4).
// Reads vertex colors from SSBO, writes to 3D voxel texture.

const char* COMPUTE_SHADER_SRC = R"(
#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 4) in;
layout(rgba8, binding = 0) uniform image3D voxel_texture;

uniform vec4  tet_v[40][4];
uniform vec4  tet_n[40];
uniform int   n_tets;
uniform int   res;

uniform vec4  eye_pos;
uniform vec4  eye_look;
uniform vec4  eye_b1;
uniform vec4  eye_b2;
uniform vec4  eye_b3;

layout(std430, binding = 2) buffer LightBuffer {
    vec4 vert_colors[];
};

bool barycentric(vec4 p, vec4 v0, vec4 v1, vec4 v2, vec4 v3,
                 out float s, out float t, out float u)
{
    vec4 e1=v1-v0; vec4 e2=v2-v0; vec4 e3=v3-v0; vec4 b=p-v0;
    float a00=dot(e1,e1); float a01=dot(e1,e2); float a02=dot(e1,e3);
    float a11=dot(e2,e2); float a12=dot(e2,e3); float a22=dot(e3,e3);
    float r0=dot(e1,b); float r1=dot(e2,b); float r2=dot(e3,b);
    float det=a00*(a11*a22-a12*a12)-a01*(a01*a22-a12*a02)+a02*(a01*a12-a11*a02);
    if (abs(det)<1e-10) return false;
    s=(r0*(a11*a22-a12*a12)-a01*(r1*a22-a12*r2)+a02*(r1*a12-a11*r2))/det;
    t=(a00*(r1*a22-a12*r2)-r0*(a01*a22-a12*a02)+a02*(a01*r2-r1*a02))/det;
    u=(a00*(a11*r2-r1*a12)-a01*(a01*r2-r1*a02)+r0*(a01*a12-a11*a02))/det;
    vec4 residual=b-s*e1-t*e2-u*e3;
    if (dot(residual,residual)>1e-8) return false;
    float eps=1e-6;
    return s>=-eps && t>=-eps && u>=-eps && (s+t+u)<=1.0+eps;
}

void main() {
    ivec3 idx = ivec3(gl_GlobalInvocationID.xyz);
    if (idx.x >= res || idx.y >= res || idx.z >= res) return;

    float fi=(float(idx.x)/float(res-1))*2.0-1.0;
    float fj=(float(idx.y)/float(res-1))*2.0-1.0;
    float fk=(float(idx.z)/float(res-1))*2.0-1.0;

    vec4 p     = eye_pos - fi*eye_b1 - fj*eye_b2 - fk*eye_b3;
    vec4 focal = eye_pos + eye_look;
    vec4 d     = focal - p;

    float best_t     = 1e38;
    vec4  best_color = vec4(0.0);
    bool  hit        = false;

    for (int i = 0; i < n_tets; i++) {
        vec4 n_hat = tet_n[i];
        float denom = dot(n_hat, d);
        if (abs(denom) < 1e-10) continue;

        float C = dot(n_hat, tet_v[i][0]);
        float t = (C - dot(n_hat, p)) / denom;
        if (t <= 1.0 + 1e-6) continue;
        if (t >= best_t) continue;

        vec4 hit_pt = p + t * d;
        float s, tv, u;
        if (!barycentric(hit_pt, tet_v[i][0], tet_v[i][1],
                         tet_v[i][2], tet_v[i][3], s, tv, u)) continue;

        float w = 1.0 - s - tv - u;
        vec3 c0 = vert_colors[i*4+0].rgb;
        vec3 c1 = vert_colors[i*4+1].rgb;
        vec3 c2 = vert_colors[i*4+2].rgb;
        vec3 c3 = vert_colors[i*4+3].rgb;
        vec3 color = w*c0 + s*c1 + tv*c2 + u*c3;

        best_t     = t;
        best_color = vec4(color, 0.1);
        hit        = true;
    }

    // ground plane
    if (abs(d.y) > 1e-10) {
        float t_ground = -p.y / d.y;
        if (t_ground > 1.0 && t_ground < best_t) {
            best_t     = t_ground;
            best_color = vec4(0.2, 0.5, 0.1, 0.05);
            hit        = true;
        }
    }

    if (!hit) {
        best_color = vec4(0.4, 0.6, 0.9, 0.05);
    }

    imageStore(voxel_texture, idx, best_color);
}
)";

// ── Vertex shader ─────────────────────────────────────────────
// Fullscreen quad — positions hardcoded, no VBO needed.

const char* VERT_SHADER_SRC = R"(
#version 430

const vec2 POSITIONS[6] = vec2[6](
    vec2(-1,-1), vec2( 1,-1), vec2( 1, 1),
    vec2(-1,-1), vec2( 1, 1), vec2(-1, 1)
);

out vec2 clip_pos;

void main() {
    clip_pos    = POSITIONS[gl_VertexID];
    gl_Position = vec4(clip_pos, 0.0, 1.0);
}
)";

// ── Fragment shader ───────────────────────────────────────────
// Ray marches the 3D voxel texture for each screen pixel.

const char* FRAG_SHADER_SRC = R"(
#version 430

in  vec2 clip_pos;
out vec4 frag_color;

uniform sampler3D voxel_tex;

uniform vec3  cam_pos;
uniform vec3  cam_forward;
uniform vec3  cam_right;
uniform vec3  cam_up;
uniform float aspect;
uniform float fov_scale;
uniform float extinction;
uniform float step_size;

void main() {
    vec3 ray_dir = normalize(
        cam_forward
        + clip_pos.x * aspect * fov_scale * cam_right
        + clip_pos.y *          fov_scale * cam_up
    );
    vec3 ray_orig = cam_pos;

    vec3 inv_dir = 1.0 / ray_dir;
    vec3 t_lo    = (-1.0 - ray_orig) * inv_dir;
    vec3 t_hi    = ( 1.0 - ray_orig) * inv_dir;
    vec3 t_min_v = min(t_lo, t_hi);
    vec3 t_max_v = max(t_lo, t_hi);

    float t_near = max(max(t_min_v.x, t_min_v.y), t_min_v.z);
    float t_far  = min(min(t_max_v.x, t_max_v.y), t_max_v.z);

    if (t_far < 0.0 || t_near > t_far) {
        frag_color = vec4(0.05, 0.05, 0.08, 1.0);
        return;
    }

    float t         = max(t_near, 0.0);
    float step_alpha = 1.0 - exp(-extinction * step_size);

    vec3  acc_color = vec3(0.0);
    float acc_alpha = 0.0;

    while (t < t_far && acc_alpha < 0.99) {
        vec3 world_pos = ray_orig + t * ray_dir;
        vec3 tex_coord = world_pos * 0.5 + 0.5;
        vec4 voxel_sample = texture(voxel_tex, tex_coord);

        if (voxel_sample.a > 0.05) {
            float transmittance = 1.0 - acc_alpha;
            acc_color += transmittance * voxel_sample.rgb * step_alpha;
            acc_alpha += transmittance * step_alpha;
        }

        t += step_size;
    }

    frag_color = vec4(acc_color, 1.0);
}
)";

// ── Compilation utilities ─────────────────────────────────────

GLuint compile_shader(const char* src, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile error:\n") + log);
    }
    return shader;
}

GLuint make_compute_program(const char* src) {
    GLuint shader  = compile_shader(src, GL_COMPUTE_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Program link error:\n") + log);
    }
    return program;
}

GLuint make_render_program(const char* vert_src, const char* frag_src) {
    GLuint vert    = compile_shader(vert_src, GL_VERTEX_SHADER);
    GLuint frag    = compile_shader(frag_src, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string("Render program link error:\n") + log);
    }
    return program;
}
