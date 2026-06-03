# FDRender

### A real-time 4D volumetric renderer

---

## Have you ever wondered what it would be like to see through the eyes of a 4D creature?

Think about how your eye works. You live in 3D, your retina is a 2D surface, and light from your 3D world lands on that flat surface. That 2D image is your perception of reality.

Go down a dimension. A 2D creature has a 1D retina — a line. That's its entire window onto its flat world. It lives inside its 2D image space and has no vantage point above it. To perceive the whole scene at once it would need to see through the image — a semi-transparent fog — so every layer is visible through every other layer simultaneously.

Now go up a dimension.

A 4D creature has a 3D retina — a volume. Light from its 4D world lands in that entire 3D volume. It has a vantage point in the fourth dimension and can perceive that whole volume at once. We don't have that vantage point. We live inside the volume. So we do exactly what the 2D creature had to do — we look into the fog from within and gather all the information at the same time.

This engine renders that 3D retinal volume in real time. You are not looking at a 4D object projected onto a screen. You are looking at what a 4D creature actually sees — its full perception of its world, made legible to a 3D brain by spreading it out as volumetric fog.

**You are perceiving the perception of a 4D world.**

---

## How it works

FDRender uses a two-stage GPU pipeline to construct and display the 4D retinal volume.

**Stage 1 — 4D → 3D (Compute Shader)**

A compute shader casts rays from each point in a 3D voxel grid into 4D space. Each voxel corresponds to one point on the 4D creature's retina. The ray finds the nearest intersecting tetrahedral face of the 4D geometry, computes lighting via a separate GPU lighting pass, and stores the resulting color in a 3D texture. The ground plane and sky are handled here as well, so the full scene — geometry, ground, and sky — is baked into the texture every frame.

Lighting is computed in a dedicated compute shader pass that runs once per vertex per frame, writing results to a GPU buffer. The ray cast shader reads from this buffer — no per-voxel lighting computation needed.

**Stage 2 — 3D → 2D (Fragment Shader)**

A fullscreen quad pass ray marches through the 3D voxel texture for each screen pixel. The ray marcher uses the slab method to find where each screen ray enters and exits the voxel volume, then accumulates color and opacity front-to-back using standard volumetric compositing. Trilinear interpolation from the GPU texture sampler gives smooth results even at moderate voxel resolutions.

**4D Navigation**

The 4D eye has a position, three basis vectors defining the retinal volume orientation, and a focal point. Full 4D navigation is supported — translation along all four axes and rotation in the XZ, XW, and ZW planes. A 3D orbital camera separately controls the viewing angle on the fog volume itself, letting you inspect the retinal image from any angle.

**Scene Format**

4D geometry is defined in `.fdr` (Four Dimensional Render) files — a human-readable text format specifying vertices in 4D space, tetrahedral index groups with correct winding order for outward normals, and per-object color and opacity.

---

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | Orbit 3D viewer |
| Q / E | Zoom 3D viewer |
| I / K | Move forward / backward in 4D |
| J / L | Move left / right in 4D |
| U / O | Move kata / ana in 4D |
| T / Y | Rotate XZ plane |
| G / H | Rotate XW plane |
| B / N | Rotate ZW plane |
| Escape | Quit |

---

## Requirements

- OpenGL 4.3+
- GLFW
- GLAD
- GLM
- C++17

Build with CMake:
```
mkdir build && cd build
cmake ..
make
./FDRender
```

---

## Roadmap

**Rendering**
- Rasterization pipeline replacing ray cast for improved performance at scale
- Adaptive step size ray marching with spatial coherence between neighboring pixels
- Shadow mapping from directional sun light
- Inside and outside face normals for hollow geometry
- Smooth geometry via higher order primitives

**Geometry**
- Hypersphere, hypercylinder, hypercone primitives
- Simple geometry editor using cross-section interpolation — define a 4D object as a sequence of 3D cross sections and interpolate between them
- Animated geometry and skeletal deformation in 4D

**Navigation and UI**
- Navball HUD showing orientation in hyperspherical coordinates with horizon surface indicator
- Full 6-degree-of-freedom 4D navigation including up/down and all rotational planes
- Minimap of the 4D scene

**Physics and Game**
- 4D rigid body physics and collision detection
- Reinforcement learning locomotion for 4D creatures
- First person 4D game built on this engine

---

## Background

This project started as a question: what would a 4D creature actually see? Not a mathematician's projection of 4D structure onto 2D, but a genuine model of 4D visual perception — the equivalent of how a 3D creature's 2D retina receives a 2D image of its 3D world.

The answer is a 3D retinal volume. This engine renders that volume in real time using GPU compute shaders, making it possible to navigate a 4D scene and observe it the way its native inhabitants would.

No prior implementation of this approach is known to exist.

---

## License

MIT
