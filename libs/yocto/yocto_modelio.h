//
// # Yocto/ModelIO: Serialization for Obj, Ply, Stl and Pbrt models
//
// Yocto/ModelIO is a collection of utilities for loading and saving scenes
// and meshes in Ply, Obj, Stl and Pbrt formats.
// Yocto/ModelIO is implemented in `yocto_modelio.h` and `yocto_modelio.cpp`,
// and depends on `fast_float.h` for number parsing.
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

#ifndef _YOCTO_MODELIO_H_
#define _YOCTO_MODELIO_H_

// -----------------------------------------------------------------------------
// INCLUDES
// -----------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

#include "yocto_math.h"

// -----------------------------------------------------------------------------
// USING DIRECTIVES
// -----------------------------------------------------------------------------
namespace yocto {

// using directives
using std::array;
using std::string;
using std::vector;

}  // namespace yocto

// -----------------------------------------------------------------------------
// PLY LOADER AND WRITER
// -----------------------------------------------------------------------------
namespace yocto {

// Ply type
enum struct ply_type { i8, i16, i32, i64, u8, u16, u32, u64, f32, f64 };

// Ply property
struct ply_property {
  // description
  string   name    = "";
  bool     is_list = false;
  ply_type type    = ply_type::f32;

  // data
  vector<int8_t>   data_i8  = {};
  vector<int16_t>  data_i16 = {};
  vector<int32_t>  data_i32 = {};
  vector<int64_t>  data_i64 = {};
  vector<uint8_t>  data_u8  = {};
  vector<uint16_t> data_u16 = {};
  vector<uint32_t> data_u32 = {};
  vector<uint64_t> data_u64 = {};
  vector<float>    data_f32 = {};
  vector<double>   data_f64 = {};

  // list length
  vector<uint8_t> ldata_u8 = {};
};

// Ply elements
struct ply_element {
  // element content
  string               name       = "";
  size_t               count      = 0;
  vector<ply_property> properties = {};
};

// Ply format
enum struct ply_format { ascii, binary_little_endian, binary_big_endian };

// Ply model
struct ply_model {
  // ply content
  ply_format          format   = ply_format::binary_little_endian;
  vector<string>      comments = {};
  vector<ply_element> elements = {};
};

// Load and save ply
[[nodiscard]] bool load_ply(
    const string& filename, ply_model& ply, string& error);
[[nodiscard]] bool save_ply(
    const string& filename, const ply_model& ply, string& error);

// Get ply properties
bool has_property(
    const ply_model& ply, const string& element, const string& property);

bool get_value(const ply_model& ply, const string& element,
    const string& property, vector<float>& values);
bool get_values(const ply_model& ply, const string& element,
    const array<string, 2>& properties, vector<vec4f>& values);
bool get_values(const ply_model& ply, const string& element,
    const array<string, 3>& properties, vector<vec3f>& values);
bool get_values(const ply_model& ply, const string& element,
    const array<string, 4>& properties, vector<vec4f>& values);
bool get_values(const ply_model& ply, const string& element,
    const array<string, 12>& properties, vector<frame3f>& values);

bool get_lists(const ply_model& ply, const string& element,
    const string& property, vector<vector<int>>& lists);
bool get_list_sizes(const ply_model& ply, const string& element,
    const string& property, vector<byte>& sizes);
bool get_list_values(const ply_model& ply, const string& element,
    const string& property, vector<int>& values);

// Get ply properties for meshes
bool get_positions(const ply_model& ply, vector<vec3f>& values);
bool get_normals(const ply_model& ply, vector<vec3f>& values);
bool get_texcoords(
    const ply_model& ply, vector<vec2f>& values, bool flipv = false);
bool get_colors(const ply_model& ply, vector<vec3f>& values);
bool get_colors(const ply_model& ply, vector<vec4f>& values);
bool get_radius(const ply_model& ply, vector<float>& values);
bool get_faces(const ply_model& ply, vector<vector<int>>*& faces);
bool get_lines(const ply_model& ply, vector<vec2i>& lines);
bool get_points(const ply_model& ply, vector<int>& points);
bool get_triangles(const ply_model& ply, vector<vec3i>& triangles);
bool get_quads(const ply_model& ply, vector<vec4i>& quads);
bool get_faces(
    const ply_model& ply, vector<vec3i>& triangles, vector<vec4i>& quads);
bool has_quads(const ply_model& ply);

// Add ply properties
bool add_value(ply_model& ply, const string& element, const string& property,
    const vector<float>& values);
bool add_values(ply_model& ply, const string& element,
    const array<string, 2>& properties, const vector<vec2f>& values);
bool add_values(ply_model& ply, const string& element,
    const array<string, 3>& properties, const vector<vec3f>& values);
bool add_values(ply_model& ply, const string& element,
    const array<string, 4>& properties, const vector<vec4f>& values);
bool add_values(ply_model& ply, const string& element,
    const array<string, 12>& properties, const vector<frame3f>& values);

bool add_value(ply_model& ply, const string& element, const string& property,
    const vector<int>& values);
bool add_values(ply_model& ply, const string& element,
    const array<string, 2>& properties, const vector<vec2i>& values);
bool add_values(ply_model& ply, const string& element,
    const array<string, 3>& properties, const vector<vec3i>& values);
bool add_values(ply_model& ply, const string& element,
    const array<string, 4>& properties, const vector<vec4i>& values);

bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vector<int>>& values);
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<byte>& sizes, const vector<int>& values);
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<int>& values);
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vec2i>& values);
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vec3i>& values);
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vec4i>& values);

// Add ply properties for meshes
bool add_positions(ply_model& ply, const vector<vec3f>& values);
bool add_normals(ply_model& ply, const vector<vec3f>& values);
bool add_texcoords(
    ply_model& ply, const vector<vec2f>& values, bool flipv = false);
bool add_colors(ply_model& ply, const vector<vec3f>& values);
bool add_colors(ply_model& ply, const vector<vec4f>& values);
bool add_radius(ply_model& ply, const vector<float>& values);
bool add_faces(ply_model& ply, const vector<vector<int>>& values);
bool add_faces(
    ply_model& ply, const vector<vec3i>& triangles, const vector<vec4i>& quads);
bool add_triangles(ply_model& ply, const vector<vec3i>& triangles);
bool add_quads(ply_model& ply, const vector<vec4i>& quads);
bool add_lines(ply_model& ply, const vector<vec2i>& lines);
bool add_points(ply_model& ply, const vector<int>& points);

}  // namespace yocto

// -----------------------------------------------------------------------------
// OBJ LOADER AND WRITER
// -----------------------------------------------------------------------------
namespace yocto {

// Obj vertex
struct obj_vertex {
  int position = 0;
  int texcoord = 0;
  int normal   = 0;
};

inline bool operator==(const obj_vertex& a, const obj_vertex& b) {
  return a.position == b.position && a.texcoord == b.texcoord &&
         a.normal == b.normal;
}

// Obj element type
enum struct obj_etype : uint16_t { face, line, point };

// Obj element
struct obj_element {
  uint16_t  size     = 0;
  obj_etype etype    = obj_etype::face;
  int       material = 0;
};

// Obj texture information.
struct obj_texture {
  string path  = "";     // file path
  bool   clamp = false;  // clamp to edge
  float  scale = 1;      // scale for bump/displacement

  obj_texture() = default;
  explicit obj_texture(const string& path) : path{path} {}
};

// Obj material
struct obj_material {
  // material name and type
  string name  = "";
  int    illum = 0;

  // material colors and values
  vec3f emission     = {0, 0, 0};
  vec3f ambient      = {0, 0, 0};
  vec3f diffuse      = {0, 0, 0};
  vec3f specular     = {0, 0, 0};
  vec3f reflection   = {0, 0, 0};
  vec3f transmission = {0, 0, 0};
  float exponent     = 10;
  float ior          = 1.5;
  float opacity      = 1;

  // material textures
  int emission_tex     = -1;
  int ambient_tex      = -1;
  int diffuse_tex      = -1;
  int specular_tex     = -1;
  int reflection_tex   = -1;
  int transmission_tex = -1;
  int exponent_tex     = -1;
  int opacity_tex      = -1;
  int bump_tex         = -1;
  int normal_tex       = -1;
  int displacement_tex = -1;
};

// Obj shape
struct obj_shape {
  string              name      = "";
  vector<vec3f>       positions = {};
  vector<vec3f>       normals   = {};
  vector<vec2f>       texcoords = {};
  vector<obj_vertex>  vertices  = {};
  vector<obj_element> elements  = {};
};

// Obj camera
struct obj_camera {
  string  name     = "";
  frame3f frame    = identity3x4f;
  bool    ortho    = false;
  float   aspect   = 16.0f / 9.0f;
  float   lens     = 0.50f;
  float   film     = 0.036f;
  float   focus    = 0;
  float   aperture = 0;
};

// Obj environment
struct obj_environment {
  string  name         = "";
  frame3f frame        = identity3x4f;
  vec3f   emission     = {0, 0, 0};
  int     emission_tex = -1;
};

// Obj model
struct obj_model {
  vector<string>          comments     = {};
  vector<obj_shape>       shapes       = {};
  vector<obj_material>    materials    = {};
  vector<obj_texture>     textures     = {};
  vector<obj_camera>      cameras      = {};
  vector<obj_environment> environments = {};
};

// Load and save obj shape
obj_shape load_sobj(const string& filename, bool face_varying = false);
void      load_obj(
         const string& filename, obj_shape& obj, bool face_varying = false);
void save_obj(const string& filename, const obj_shape& obj);

// Load and save obj
[[nodiscard]] bool load_obj(const string& filename, obj_model& obj,
    string& error, bool face_varying = false, bool split_materials = false);
[[nodiscard]] bool save_obj(
    const string& filename, const obj_model& obj, string& error);

// Load and save obj shape
[[nodiscard]] bool load_obj(const string& filename, obj_shape& obj,
    string& error, bool face_varying = false);
[[nodiscard]] bool save_obj(
    const string& filename, const obj_shape& obj, string& error);

// Get obj shape.
void get_positions(const obj_shape& shape, vector<vec3f>& positions);
void get_normals(const obj_shape& shape, vector<vec3f>& normals);
void get_texcoords(
    const obj_shape& shape, vector<vec2f>& texcoords, bool flipv = false);
void get_faces(const obj_shape& shape, vector<vec3i>& triangles,
    vector<vec4i>& quads, vector<int>& materials);
void get_triangles(
    const obj_shape& shape, vector<vec3i>& triangles, vector<int>& materials);
void get_quads(
    const obj_shape& shape, vector<vec4i>& quads, vector<int>& materials);
void get_lines(
    const obj_shape& shape, vector<vec2i>& lines, vector<int>& materials);
void get_points(
    const obj_shape& shape, vector<int>& points, vector<int>& materials);
void get_fvquads(const obj_shape& shape, vector<vec4i>& quadspos,
    vector<vec4i>& quadsnorm, vector<vec4i>& quadstexcoord,
    vector<int>& materials);
void get_faces(const obj_shape& shape, int material, vector<vec3i>& triangles,
    vector<vec4i>& quads);
void get_triangles(
    const obj_shape& shape, int material, vector<vec3i>& triangles);
void get_quads(const obj_shape& shape, int material, vector<vec4i>& quads);
void get_lines(const obj_shape& shape, int material, vector<vec2i>& lines);
void get_points(const obj_shape& shape, int material, vector<int>& points);
bool has_quads(const obj_shape& shape);

// get unique materials from shape
vector<int> get_materials(const obj_shape& shape);

// Add obj shape
void add_positions(obj_shape& shape, const vector<vec3f>& positions);
void add_normals(obj_shape& shape, const vector<vec3f>& normals);
void add_texcoords(
    obj_shape& shape, const vector<vec2f>& texcoords, bool flipv = false);
void add_triangles(obj_shape& shape, const vector<vec3i>& triangles,
    int material, bool has_normals, bool has_texcoord);
void add_quads(obj_shape& shape, const vector<vec4i>& quads, int material,
    bool has_normals, bool has_texcoord);
void add_lines(obj_shape& shape, const vector<vec2i>& lines, int material,
    bool has_normals, bool has_texcoord);
void add_points(obj_shape& shape, const vector<int>& points, int material,
    bool has_normals, bool has_texcoord);
void add_fvquads(obj_shape& shape, const vector<vec4i>& quadspos,
    const vector<vec4i>& quadsnorm, const vector<vec4i>& quadstexcoord,
    int material);

}  // namespace yocto

// -----------------------------------------------------------------------------
// HELPER FOR DICTIONARIES
// -----------------------------------------------------------------------------
namespace std {

// Hash functor for vector for use with hash_map
template <>
struct hash<yocto::obj_vertex> {
  size_t operator()(const yocto::obj_vertex& v) const {
    const std::hash<int> hasher = std::hash<int>();
    auto                 h      = (size_t)0;
    h ^= hasher(v.position) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= hasher(v.normal) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= hasher(v.texcoord) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};

}  // namespace std

// -----------------------------------------------------------------------------
// PBRT LOADER AND WRITER
// -----------------------------------------------------------------------------
namespace yocto {

struct stl_shape {
  vector<vec3f> positions = {};
  vector<vec3i> triangles = {};
  vector<vec3f> fnormals  = {};
};

struct stl_model {
  vector<stl_shape> shapes = {};
};

// Load/save stl
[[nodiscard]] bool load_stl(const string& filename, stl_model& stl,
    string& error, bool unique_vertices = true);
[[nodiscard]] bool save_stl(const string& filename, const stl_model& stl,
    string& error, bool ascii = false);

// Get/set data
bool get_triangles(const stl_model& stl, int shape_id, vector<vec3i>& triangles,
    vector<vec3f>& positions, vector<vec3f>& fnormals);
void add_triangles(stl_model& stl, const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3f>& fnormals);

}  // namespace yocto

// -----------------------------------------------------------------------------
// PBRT LOADER AND WRITER
// -----------------------------------------------------------------------------
namespace yocto {

// Pbrt camera
struct pbrt_camera {
  // camera parameters
  frame3f frame      = identity3x4f;
  frame3f frend      = identity3x4f;
  vec2i   resolution = {0, 0};
  float   lens       = 0;
  float   aspect     = 0;
  float   focus      = 0;
  float   aperture   = 0;
};

// Pbrt material
struct pbrt_texture {
  string name     = "";
  vec3f  constant = {1, 1, 1};
  string filename = "";
};

// Pbrt material type (simplified and only for the materials that matter here)
enum struct pbrt_mtype {
  // clang-format off
  matte, plastic, metal, glass, thinglass, subsurface
  // clang-format on
};

// Pbrt material
struct pbrt_material {
  string     name            = "";
  pbrt_mtype type            = pbrt_mtype::matte;
  vec3f      emission        = {0, 0, 0};
  vec3f      color           = {0, 0, 0};
  float      roughness       = 0;
  float      ior             = 1.5f;
  float      opacity         = 1;
  int        color_tex       = -1;
  vec3f      volmeanfreepath = {0, 0, 0};
  vec3f      volscatter      = {0, 0, 0};
  float      volscale        = 0.01f;
};

// Pbrt shape
struct pbrt_shape {
  frame3f         frame     = identity3x4f;
  frame3f         frend     = identity3x4f;
  bool            instanced = false;
  vector<frame3f> instances = {};
  vector<frame3f> instaends = {};
  int             material  = -1;
  string          filename_ = "";
  vector<vec3f>   positions = {};
  vector<vec3f>   normals   = {};
  vector<vec2f>   texcoords = {};
  vector<vec3i>   triangles = {};
};

// Pbrt lights
struct pbrt_light {
  frame3f       frame          = identity3x4f;
  frame3f       frend          = identity3x4f;
  vec3f         emission       = {0, 0, 0};
  vec3f         from           = {0, 0, 0};
  vec3f         to             = {0, 0, 0};
  bool          distant        = false;
  vec3f         area_emission  = {0, 0, 0};
  frame3f       area_frame     = identity3x4f;
  frame3f       area_frend     = identity3x4f;
  vector<vec3i> area_triangles = {};
  vector<vec3f> area_positions = {};
  vector<vec3f> area_normals   = {};
};
struct pbrt_environment {
  frame3f frame        = identity3x4f;
  frame3f frend        = identity3x4f;
  vec3f   emission     = {0, 0, 0};
  int     emission_tex = -1;
};

// Pbrt model
struct pbrt_model {
  // pbrt data
  vector<string>           comments     = {};
  vector<pbrt_camera>      cameras      = {};
  vector<pbrt_shape>       shapes       = {};
  vector<pbrt_environment> environments = {};
  vector<pbrt_light>       lights       = {};
  vector<pbrt_material>    materials    = {};
  vector<pbrt_texture>     textures     = {};
};

// Load/save pbrt
[[nodiscard]] bool load_pbrt(const string& filename, pbrt_model& pbrt,
    string& error, bool ply_meshes = false);
[[nodiscard]] bool save_pbrt(const string& filename, const pbrt_model& pbrt,
    string& error, bool ply_meshes = false);

}  // namespace yocto

#endif
