WALDEM ENGINE

![image](https://github.com/JoymanGD/Waldem/blob/master/img.png?raw=true)

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
* ImGuizmo
* Mesh selection using MeshIdsRenderTarget

Renderer:
* Ray tracing
* PBR
* GBuffer
* Deferred
* Forward
* Shadow mapping
* Materials
* FFT Ocean simulation

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
