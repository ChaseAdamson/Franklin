#include "fdr_parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <regex>

// ── Helpers ───────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

static bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

// Parse a line like "[ 1, -1,  1,  1]" into 4 floats
static Vec4 parse_vec4(const std::string& line) {
    std::string s = line;
    // strip brackets and commas
    for (char& c : s) if (c == '[' || c == ']' || c == ',') c = ' ';
    std::istringstream ss(s);
    Vec4 v;
    if (!(ss >> v.x >> v.y >> v.z >> v.w))
        throw std::runtime_error("Failed to parse Vec4 from: " + line);
    return v;
}

// Parse a line like "[ 7,  3,  5,  6]" into 4 ints
static Tet parse_tet(const std::string& line) {
    std::string s = line;
    for (char& c : s) if (c == '[' || c == ']' || c == ',') c = ' ';
    std::istringstream ss(s);
    Tet t;
    if (!(ss >> t.v[0] >> t.v[1] >> t.v[2] >> t.v[3]))
        throw std::runtime_error("Failed to parse Tet from: " + line);
    return t;
}

// Parse a line like "[ 180, 100, 40, 0.15]" into RGBA (0-1 range for rgb)
static RGBA parse_rgba(const std::string& line) {
    std::string s = line;
    for (char& c : s) if (c == '[' || c == ']' || c == ',') c = ' ';
    std::istringstream ss(s);
    RGBA rgba;
    if (!(ss >> rgba.r >> rgba.g >> rgba.b >> rgba.a))
        throw std::runtime_error("Failed to parse RGBA from: " + line);
    // convert 0-255 rgb to 0-1
    rgba.r /= 255.0f;
    rgba.g /= 255.0f;
    rgba.b /= 255.0f;
    return rgba;
}

// ── Main parser ───────────────────────────────────────────────

FDRObject load_fdr(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + filepath);

    FDRObject obj;
    obj.name = filepath;

    // Parse state
    enum class Section { None, Verts, Tets, RGBA };
    Section section = Section::None;

    float r_global = 255, g_global = 255, b_global = 255, a_global = 1.0f;
    bool has_globals = false;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);

        // skip comments and empty lines
        if (line.empty() || starts_with(line, "//")) continue;
        if (starts_with(line, "// End") || starts_with(line, "//End")) break;

        // section headers
        if (starts_with(line, "Verts"))  { section = Section::Verts; continue; }
        if (starts_with(line, "Tets"))   { section = Section::Tets;  continue; }
        if (starts_with(line, "RGBA"))   { section = Section::RGBA;  continue; }
        if (line == "{" || line == "}") continue;

        // global color values
        if (starts_with(line, "R_global")) {
            sscanf(line.c_str(), "R_global = %f", &r_global);
            has_globals = true; continue;
        }
        if (starts_with(line, "G_global")) {
            sscanf(line.c_str(), "G_global = %f", &g_global);
            continue;
        }
        if (starts_with(line, "B_global")) {
            sscanf(line.c_str(), "B_global = %f", &b_global);
            continue;
        }
        if (starts_with(line, "A_global")) {
            sscanf(line.c_str(), "A_global = %f", &a_global);
            continue;
        }

        // section data
        switch (section) {
            case Section::Verts:
                if (starts_with(line, "["))
                    obj.vertices.push_back(parse_vec4(line));
                break;

            case Section::Tets:
                if (starts_with(line, "["))
                    obj.tetrahedra.push_back(parse_tet(line));
                break;

            case Section::RGBA:
                if (line == "None") {
                    // use global — handled below
                } else if (starts_with(line, "[")) {
                    obj.rgba_per_tet.push_back(parse_rgba(line));
                    obj.has_per_tet_color = true;
                }
                break;

            default: break;
        }
    }

    // set global color
    obj.rgba_global = { r_global / 255.0f, g_global / 255.0f,
                        b_global / 255.0f, a_global };

    return obj;
}
