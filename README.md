WALDEM ENGINE - custom engine written in C++

![alt text](https://raw.githubusercontent.com/JoymanGD/Waldem/refs/heads/master/img.png "Waldem Engine screenshot")

Architecture:
* ECS
* Cross Render API (DirectX)
* Cross OS (Windows)
* Layers (Game, Editor, Debug)
* Editor widgets structure
* Separate game and engine structure

Editor:
* Entities list widget
* Entity details widget
* Content browser widget
* ImGuizmo
* Mesh selection using MeshIdsRenderTarget

Renderer:
* Fully bindless
* Indirect draw
* Ray tracing
* PBR
* GBuffer
* Deferred
* Forward
* Shadow mapping
* Materials
* FFT Ocean simulation
* Sprite rendering
* Particle system

Audio:
* Audio listeners and sources
* Spatial (3D) audio

Physics:
* AABB BVH broadphase collisions
* GJK narrowphase collision detection
* EPA
* Collision resolving [WIP]
* Physics system [WIP]

Import:
* GLTF/FBX
* Image
* Wav

Other:
* Logging system
* GLM math
* SDL2 window and input handling
