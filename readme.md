# Yocto/Pathtrace: Tiny Volumetric Path Tracer

In this homework, you will learn how to build a simple path tracer with support
for subdivision surfaces, displacement and subsurface scattering.
In particular, you will learn how to

- handle subdivision surfaces,
- handle normal mapping (we have implemented displacement for you),
- write a path tracer with support for homogeneous volumes.

## Framework

The code uses the library [Yocto/GL](https://github.com/xelatihy/yocto-gl),
that is included in this project in the directory `yocto`.
We suggest to consult the documentation for the library that you can find
at the beginning of the header files. Also, since the library is getting improved
during the duration of the course, se suggest that you star it and watch it
on Github, so that you can notified as improvements are made.

In order to compile the code, you have to install
[Xcode](https://apps.apple.com/it/app/xcode/id497799835?mt=12)
on OsX, [Visual Studio 2019](https://visualstudio.microsoft.com/it/vs/) on Windows,
or a modern version of gcc or clang on Linux,
together with the tools [cmake](www.cmake.org) and [ninja](https://ninja-build.org).
The script `scripts/build.sh` will perform a simple build on OsX.
As discussed in class, we prefer to use
[Visual Studio Code](https://code.visualstudio.com), with
[C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) and
[CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
extensions, that we have configured to use for this course.

You will write your code in the file `yocto_pathtrace.cpp` for functions that
are declared in `yocto_pathtrace.h`. Your renderer is called by `yscenetrace.cpp`
for a command-line interface and `ysceneitraces.cpp` that show a simple
user interface.

This repository also contains tests that are executed from the command line
as shown in `run.sh`. The rendered images are saved in the `out/` directory.
The results should match the ones in the directory `check/`.

## Functionality (24 punti)

In this homework you will implement the following features:

- **Subdivision Surfaces** in function `tesselate_surface()`:
  - implement Catmull-Clark subdivision following the slides
  - while the solution should match the one in Yocto/Shape, your
    implementation _cannot_ use the Yocto one, neither calling it nor
    adapting the code – I will be able to tell since the Yocto/Shape code
    supports more features
- **Volumetric Path Tracing** in function `shade_volpathtrace()`:
  - follow the slides and implement a volumetric path tracer
  - you have to also implement all support functions
  - you can use the corresponding functions in Yocto/GL

## Extra Credit (16 punti)

As usual, here are the extra credit work you can choose to do. As before,
the maximum points sum is the one reported above. But you can do more of these
if desired.

New this time is that if you do extra credit, you have to **prepare a short PDF**
that describes which feature you have implemented and includes images for
those features. **Without the PDF the extra credit will not be graded**.
The PDF should be very short and just say what you did succinctly, like with
a bullet points lists.

- **Shadow Terminator** (simple):
  - implement the shadow terminator fix shown in
    [Raytracing Gem 2 - Chapter 4](https://link.springer.com/content/pdf/10.1007%2F978-1-4842-7185-8.pdf)
  - show images that compare Yocto/GL to your solution
- **Path Tracing Implicit** (medium):
  - write a path tracer that can render implicit surfaces
  - follow the intersection code in [] and the speedup technique in []
  - you can keep all shading as is, just change the intersection methods
- **Better Sampling Sequence** (medium):
  - add a method to generate well distributed samples based on
    [ZSampler](http://abdallagafar.com/publications/zsampler/)
  - follow the implementation in [pbrt-v4](https://github.com/mmp/pbrt-v4)
  - demonstrate the method with direct illumination of area lights
- **Volumetric Path Tracing I - Delta Tracking** in path tracer (hard):
  - integrate delta tracking for smoke and clouds, based your implementation on
    [pbrt](http://www.pbr-book.org/3ed-2018/Light_Transport_II_Volume_Rendering/Sampling_Volume_Scattering.html)
  - to integrate it, you have to implement a new `sample_transmittance()`
    that both samples the distances and returns the weight
  - implement volumetric textures, by creating a 3d `volume_data` structure
    or by using NVidia `nanovdb`
  - also you have to add volumes to the renderer, which for now can be a hack
    to either hardcode a function that makes one or use example data from OpenVDB
- **Volumetric Path Tracing II - Modernized Volumetric Scattering** (very hard):
  - implement proper volumetric scattering for heterogenous materials
  - implement volumetric textures, by creating a 3d `volume_data` structure
    or by using NVidia `nanovdb`
  - add these textures to both the SceneIO loader and the path tracer
  - implement a modern volumetric method from Algorithm 1 of [Miller et al](https://cs.dartmouth.edu/~wjarosz/publications/miller19null.html) --- ignore most of the math here since it is not that helpful
  - you can use the implementation from [pbrt-v4](https://github.com/mmp/pbrt-v4)
  - also you have to add volumes to the renderer, which for now can be a hack
    to either hardcode a function that makes one or use example data from OpenVDB
- **GPU Path Tracing** on your system (hard):
  - implement a path tracer on the GPU that is equivalent to homework 02
  - you can use any rendering API you want since it all depends on the hardware you have
  - in particular, I suggest using Metal on OsX, OptiX/Owl on NVidia hardware,
    and DirectX Ray tracing on Windows (I am not familiar with Linux enough to give suggestions)
  - follow a demo/tutorial that vendors provide with your system and modify it
    to render with Yocto with the same algorithm we used
- **Path Tracing in Python** (hard):
  - implement a path tracer in Python to see how performant the code remains
  - the path tracer should load Yocto/GL scenes so you can compare the execution times
  - to make it bearable I suggest that you
    - use numpy for all data
    - use any library you like to load the JSON scene and its components
    - write a wavefront path tracer that executes functions over all the image
      - first generate rays for all pixels
      - then intersect the scene for all rays
      - then executes the materials to generate more rays
      - rinse-and-repeat
      - use numpy buffers between stages
- **Yacta/GL** (hard):
  - Yocto/GL tries to be as small as possible to make understandable and hackable,
    but doing so it does not leverage the latest libraries
  - try to build a renderer using off-the-shelf open source libraries like
    - Intel Embree for raytracing
    - Intel Open Image Denoise for denoising
    - Pixar Open Subdiv for surfaces
    - MaterialX for materials


## Submission

To submit the homework, you need to pack a ZIP file that contains the code
you write and the images it generates, i.e. the ZIP _with only the
`yocto_pathtrace/` and `out/` directories_. _If doing extra credits also include
the PDF and and the test scenes._
If you are doing extra credit, include also `apps`, other images, and a 
**`readme.md`** file that describes the list of implemented extra credit.
The file should be called `<lastname>_<firstname>_<studentid>.zip`
(`<cognome>_<nome>_<matricola>.zip`) and you should exclude
all other directories. Send it on Google Classroom.
