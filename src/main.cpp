#include "renderer.h"
#include "fdr_parser.h"
#include <cstdio>
#include <stdexcept>

int main(int argc, char* argv[]) {
    // default scene file
    std::string scene_file = "assets/Tesseract.fdr";
    if (argc > 1) scene_file = argv[1];

    try {
        printf("Loading scene: %s\n", scene_file.c_str());
        FDRObject scene = load_fdr(scene_file);
        printf("Loaded %zu vertices, %zu tets\n",
               scene.vertices.size(), scene.tetrahedra.size());

        Renderer renderer;
        renderer.run(scene);

    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }

    return 0;
}
