# Project Overview

The Violet Engine is a Vulkan-based 3D rendering engine. Violet is in a very early stage of development, and the name is somewhat tentative. It currently lacks the flexibility and feature-set to be used in any serious projects, although I am hoping to change this soon.

For the time being, Violet's development is and will be driven by my own learning interests, as well as my own personal projects for which I plan to use the engine; however, anyone else is free to look at the code and use or modify it as they wish.

I am still unsure what form Violet will eventually take; it may end up as a fully featured game engine, or as a library to be included in other projects. Currently, the repository has a main() entry point, and builds as an executable to test the functionality that has been implemented thus far.

# Design Philosophy and State of the Project

Violet is being designed to allow for quick experimentation with different rendering algorithms. The intention is for Violet to take care of as many common tasks as possible, and in a way that performs well, while still having the flexibility to be used with any conceivable set of rendering algorithms.

One of the key features is automatic building and execution of dependency graphs for the various processes that use the GPU in rendering and presenting frames. In other words, Violet will handle the creation and usage of semaphores, and make sure processes are run in the correct order.

Violet also has facilities to handle vertex and transform data of various meshes, using the GPUMesh and GPUMeshWrangler classes; although, much like the rest of Violet, these are very early in development, and in need of additional features and optimization.

I plan to add support for occlusion culling, although I need to investigate what occlusion culling algorithms I want to use.

# File and Class Structure

Currently, all first-party C++ source and header files are located in the `src` directory, while all GLSL shader source files are located in the `src/shaders` directory. All third-party code is located in the `thirdparty` directory. The entry point is `int main()` at line 21 of main.cpp.

Most classes have one header (.h file) and one implementation file (.cpp file) which share the name of the class. However, some very closely-related classes share a header and implementation file.

The `GPUEngine` class is responsible for managing the most basic resources, and facilitates communication between other classes. There are several classes which have a single instance, which the `GPUEngine` either owns or holds a non-owning reference to, and which other classes use the `GPUEngine` to access.

The `GPUProcess` class is a base class which is used to abstract the various steps of rendering which must be properly synchronized. A `GPUProcess` child class instance performs one of those steps, and may use resources owned by one or more other `GPUProcess` child class instances. `GPUProcess::PassableResource` and `GPUProcess::PRDependency` are used to communicate and describe these dependencies.

The `GPUDependencyGraph` class is responsible for managing all of the active `GPUProcess` child class instances, and any dependencies they have on each other's passable resources. `GPUProcessDependencyGraph` creates an executable sequence of these processes, with proper synchronization between processes which depend on each other. `GPUDependencyGraph` owns all `GPUProcess` instances which are added to it. A `GPUDependencyGraph` instance is created and owned by the `GPUEngine`.

The `GPUMesh` class loads 3D mesh data from a file into GPU memory, where it can then be used in rendering. A single `GPUMesh` instance represents a single 3D mesh, and owns all associated data. Multiple instances of a mesh can be rendered at once, and the `GPUMesh::Instance` class represents a single instance of a given mesh.

The `GPUMeshWrangler` class collects all data for all `GPUMesh::Instance` instances which will be rendered in a given frame, packages that data in a useful format, and transfers it to the GPU. `GPUMeshWrangler` is a child class of `GPUProcess`. A `GPUMeshWrangler` instance is created and added to the `GPUDependencyGraph` by the `GPUEngine`.

The `GPUProcessSwapchain` class allocates and owns all resources related to image presentation, and is responsible for acquiring an image to be used as a final render target on each frame. The accompanying `GPUProcessPresent` class, which shares the same header and implementation files, signals `GPUProcessSwapchain` to present the image after it has been rendered to.

The `GPUPipeline` class loads a set of compiled shaders from specified filenames and builds a graphics pipeline which uses them.

The `GPUImage` class manages resources for a single `VkImage` and associated `VkImageView`. It can have a fixed resolution, or use a multiple of the screen resolution. `GPUImage` is a child class of `GPUProcess`, allowing it to be managed by `GPUDependencyGraph`, although it does not actually perform an operation; it simply makes its `VkImageView` available for use by other processes.

The `GPUProcessRenderPass` class represents a single render pass. Currently, a `GPUProcessRenderPass` can only have one subpass. `GPUProcessRenderPass` queries the `GPUMeshWrangler` for active mesh instances and renders all of them. `GPUProcessRenderPass` owns and uses a `GPUPipeline`.

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