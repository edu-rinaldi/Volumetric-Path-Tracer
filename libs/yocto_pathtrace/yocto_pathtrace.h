//
// # Yocto/PathTrace: Tiny path tracer
//
//
// Yocto/PathTrace is a simple path tracer written on the Yocto/Scene model.
// Yocto/PathTrace is implemented in `yocto_pathtrace.h` and
// `yocto_pathtrace.cpp`.
//

//
// LICENSE:
//
// Copyright (c) 2016 -- 2021 Fabio Pellacini
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//

#ifndef _YOCTO_RAYTRACE_H_
#define _YOCTO_RAYTRACE_H_

// -----------------------------------------------------------------------------
// INCLUDES
// -----------------------------------------------------------------------------

#include <yocto/yocto_bvh.h>
#include <yocto/yocto_image.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_sampling.h>
#include <yocto/yocto_scene.h>

#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// SCENE AND RENDERING DATA
// -----------------------------------------------------------------------------
namespace yocto {

// Rendering state
struct pathtrace_state {
  int               width   = 0;
  int               height  = 0;
  int               samples = 0;
  vector<vec4f>     image   = {};
  vector<int>       hits    = {};
  vector<rng_state> rngs    = {};
};

}  // namespace yocto

// -----------------------------------------------------------------------------
// HIGH LEVEL API
// -----------------------------------------------------------------------------
namespace yocto {

// Type of tracing algorithm
enum struct pathtrace_shader_type {
  volpathtrace,  // volumetric path tracing
  pathtrace,     // path tracing
  naive,         // naive path tracing
  eyelight,      // eyelight rendering
  normal,        // normals
  texcoord,      // texcoords
  color,         // colors
};

// Options for trace functions
struct pathtrace_params {
  int                   camera     = 0;
  int                   resolution = 720;
  pathtrace_shader_type shader     = pathtrace_shader_type::pathtrace;
  int                   samples    = 512;
  int                   bounces    = 4;
  bool                  noparallel = false;
  int                   pratio     = 8;
  float                 exposure   = 0;
  bool                  filmic     = false;
};

const auto pathtrace_shader_names = vector<string>{"volpathtrace", "pathtrace",
    "naive", "eyelight", "normal", "texcoord", "color"};

// Scene lights used during rendering. These are created automatically.
struct pathtrace_light {
  int           instance     = invalidid;
  int           environment  = invalidid;
  vector<float> elements_cdf = {};
};

// Scene lights
struct pathtrace_lights {
  vector<pathtrace_light> lights = {};
};

// Initialize state.
pathtrace_state make_state(
    const scene_data& scene, const pathtrace_params& params);

// Build the bvh acceleration structure.
bvh_scene make_bvh(const scene_data& scene, const pathtrace_params& params);

// Initialize lights.
pathtrace_lights make_lights(
    const scene_data& scene, const pathtrace_params& params);

// Tesselate subdivs
void tesselate_surfaces(scene_data& scene);

// Progressively computes an image.
void pathtrace_samples(pathtrace_state& state, const scene_data& scene,
    const bvh_scene& bvh, const pathtrace_lights& lights,
    const pathtrace_params& params);

// Get resulting render
color_image get_render(const pathtrace_state& state);
void        get_render(color_image& render, const pathtrace_state& state);

}  // namespace yocto

#endif
