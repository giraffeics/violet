# Project Overview

The Violet Engine is a Vulkan-based 3D rendering engine. Violet is in a very early stage of development, and the name is somewhat tentative. It currently lacks the flexibility and feature-set to be used in any serious projects, although I am hoping to change this soon.

For the time being, Violet's development is and will be driven by my own learning interests, as well as my own personal projects for which I plan to use the engine; however, anyone else is free to look at the code and use or modify it as they wish.

I am still unsure what form Violet will eventually take; it may end up as a fully featured game engine, or as a library to be included in other projects. Currently, the repository has a main() entry point, and builds as an executable to test the functionality that has been implemented thus far.

# Design Philosophy and State of the Project

Violet is being designed to allow for quick experimentation with different rendering algorithms. The intention is for Violet to take care of as many common tasks as possible, and in a way that performs well, while still having the flexibility to be used with any conceivable set of rendering algorithms.

One of the key features is automatic building and execution of dependency graphs for the various processes that use the GPU in rendering and presenting frames. In other words, Violet will handle the creation and usage of semaphores, and make sure processes are run in the correct order.

Violet also has facilities to handle vertex and transform data of various meshes, using the GPUMesh and GPUMeshWrangler classes; although, much like the rest of Violet, these are very early in development, and in need of additional features and optimization.

I plan to add support for occlusion culling, although I need to investigate what occlusion culling algorithms I want to use.

# How To Build

Violet uses CMake 3.12 or higher, and has been tested on Ubuntu and Windows. The steps for downloading and building from source are as follows:

#### Ubuntu:

1. Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
2. Install the `xorg-dev` package using `sudo apt install xorg-dev`
3. Clone the repository and its submodules using `git clone https://github.com/giraffeics/violet.git --recursive`
4. Build the `violet` target using CMake.

#### Windows:

1. Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
2. Clone the repository and its submodules using `git clone https://github.com/giraffeics/violet.git --recursive`
3. Build the `violet` target using CMake.