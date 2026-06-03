# Building FDRender

## Dependencies

Install these before building:

- **CMake** 3.16+
- **GLFW3** — `brew install glfw` / `sudo apt install libglfw3-dev`
- **GLM** — `brew install glm` / `sudo apt install libglm-dev`
- **GLAD** — see below

## Setting up GLAD

GLAD is bundled manually since it's generated for your specific OpenGL version.

1. Go to https://glad.dav1d.de/
2. Set Language = C/C++, Specification = OpenGL, Profile = Compatibility, Version = 4.3
3. Check "Generate a loader"
4. Click Generate and download the zip
5. Extract so your tree looks like:

```
FDRender/
    third_party/
        glad/
            include/
                glad/
                    glad.h
                KHR/
                    khrplatform.h
            src/
                glad.c
```

## Build

```bash
mkdir build
cd build
cmake ..
make
./FDRender
```

Or on Windows with Visual Studio:
```
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
# open FDRender.sln in Visual Studio and build
```

## Running with a different scene

```bash
./FDRender path/to/your/scene.fdr
```
