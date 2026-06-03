# Building Franklin

## Platform Notes

> **Windows instructions have been tested and verified.** Linux and Mac instructions are provided based on standard practice but have not been tested yet — your mileage may vary.

---

## Windows (Tested)

### 1. Install Visual Studio 2022

Download from https://visualstudio.microsoft.com/vs/

During installation make sure to check **"Desktop development with C++"** workload. This provides the MSVC compiler that everything else depends on.

### 2. Install vcpkg

Open PowerShell and run:

```
cd C:\Dev
git clone https://github.com/microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### 3. Install dependencies via vcpkg

```
.\vcpkg install glfw3:x64-windows
.\vcpkg install glm:x64-windows
```

### 4. Set up GLAD

GLAD is bundled manually since it's generated for your specific OpenGL version.

1. Go to https://glad.dav1d.de/
2. Set Language = C/C++, Specification = OpenGL, Profile = Compatibility, Version = 4.3
3. Check "Generate a loader"
4. Click Generate and download the zip
5. Extract so your tree looks like:

```
Franklin/
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

### 5. Add vcpkg toolchain to CMakeLists.txt

Make sure this line is at the top of `CMakeLists.txt` right after `cmake_minimum_required`:

```cmake
set(CMAKE_TOOLCHAIN_FILE "C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
```

Adjust the path if you installed vcpkg somewhere other than `C:\Dev\vcpkg`.

### 6. Build with VS Code

1. Install the **CMake Tools** and **C/C++** extensions in VS Code
2. Open the Franklin folder in VS Code
3. `Ctrl+Shift+P` → CMake: Configure
4. Select **Visual Studio Community 2022 Release - amd64** as the kit
5. `Ctrl+Shift+P` → CMake: Build (or press F7)

### 7. Run

Open a terminal in the Franklin root folder and run:

```
.\build\Debug\FDRender.exe
```

Must be run from the Franklin root so it can find the `assets/` folder.

---

## Linux (Untested)

```bash
sudo apt install libglfw3-dev libglm-dev cmake
```

Set up GLAD as described above, then:

```bash
mkdir build && cd build
cmake ..
make
./FDRender
```

---

## Mac (Untested)

```bash
brew install glfw glm cmake
```

Set up GLAD as described above, then:

```bash
mkdir build && cd build
cmake ..
make
./FDRender
```

---

## Running with a different scene

```
./FDRender path/to/your/scene.fdr
```
