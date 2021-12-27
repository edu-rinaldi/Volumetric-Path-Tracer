#include "yocto_sdfs.h"
#include "yocto_scene.h"

namespace yocto {

// Eval all the SDFs in the scene at a given point p
sdf_result eval_sdf_scene(const scene_data& scene, const vec3f& p, float t) {
  sdf_result res;
  // Evaluate sdf from grid
  for (const auto& [idx, instance] : enumerate(scene.vol_instances)) {
    const auto& volume       = scene.volumes[instance.volume];
    float       instance_sdf = eval_sdf(
        volume, instance, transform_point(instance.frame, p), t);

    if (instance_sdf < res.result) res = {instance_sdf, (int)idx, invalidid};
  }

  // Evaluate sdf from function
  for (auto& [idx, sdfunc] : enumerate(scene.sdfs)) {
    auto sdf = sdfunc.f(transform_point(sdfunc.frame, p));
    if (sdf < res.result) res = {sdf, invalidid, (int)idx};
  }

  return res;
}


// Eval sdf from volume
float eval_sdf(const volume<float>& volume, const volume_instance& instance,
    const vec3f& p, float t) {
  
  // Grid resolution
  vec3f grid_res = {volume.whd.x, volume.whd.y, volume.whd.z};
  const auto& origin   = instance.frame.o;
 
  // Calculate sdf bbox
  auto  bbox_max      = origin + (volume.res * grid_res) * instance.scalef;
  auto  bbox_size     = (bbox_max - origin);
  float bbox_dist     = sd_box(p - (bbox_size * 0.5f), (bbox_size * 0.5f));
  // If we hit bbox ==> calculate uvw and lookup volume (each voxel is a sdfield)
  if (bbox_dist < flt_eps * t ) {
    vec3f uvw = p * 2.f / (bbox_size)-1;
    // eval_volume(..) is similar to texture(volumeSampler, uvw) in GLSL
    return eval_volume(volume, uvw) * instance.scalef;
  }
  // Return distance from bbox
  return bbox_dist;
}

// Eval normal given the scene
vec3f eval_sdf_normal(const scene_data& scene, const vec3f& p, float t) {
  const float h  = yocto::flt_eps * t;  // replace by an appropriate value
  auto        p1 = p + vec3f{1, -1, -1} * h;
  auto        p2 = p + vec3f{-1, -1, 1} * h;
  auto        p3 = p + vec3f{-1, 1, -1} * h;
  auto        p4 = p + vec3f{1, 1, 1} * h;
  return normalize(vec3f{1, -1, -1} * eval_sdf_scene(scene, p1, t).result +
                   vec3f{-1, -1, 1} * eval_sdf_scene(scene, p2, t).result +
                   vec3f{-1, 1, -1} * eval_sdf_scene(scene, p3, t).result +
                   vec3f{1, 1, 1} * eval_sdf_scene(scene, p4, t).result);
}

// Eval the normal of a sdfunction
vec3f eval_sdf_normal(const sdf_data& sdf, const vec3f& p, float t) {
  const float h = yocto::flt_eps * t;  // replace by an appropriate value
  auto        p1 = transform_point(sdf.frame, p + vec3f{1, -1, -1} * h);
  auto        p2 = transform_point(sdf.frame, p + vec3f{-1, -1, 1} * h);
  auto        p3 = transform_point(sdf.frame, p + vec3f{-1, 1, -1} * h);
  auto        p4 = transform_point(sdf.frame, p + vec3f{1, 1, 1} * h);
  return normalize(vec3f{1, -1, -1} * sdf.f(p1) + vec3f{-1, -1, 1} * sdf.f(p2) +
                   vec3f{-1, 1, -1} * sdf.f(p3) + vec3f{1, 1, 1} * sdf.f(p4));
}
// Eval normal of sdfields
vec3f eval_sdf_normal(const volume<float>& volume,
    const volume_instance& instance, const vec3f& p, float t) {
  const float h = yocto::flt_eps * t;  // replace by an appropriate value
  auto        p1 = transform_point(instance.frame, p + vec3f{1, -1, -1} * h);
  auto        p2 = transform_point(instance.frame, p + vec3f{-1, -1, 1} * h);
  auto        p3 = transform_point(instance.frame, p + vec3f{-1, 1, -1} * h);
  auto        p4 = transform_point(instance.frame, p + vec3f{1, 1, 1} * h);
  return normalize(vec3f{1, -1, -1} * eval_sdf(volume, instance, p1, t) +
      vec3f{-1, -1, 1} * eval_sdf(volume, instance, p2, t) +
      vec3f{-1, 1, -1} * eval_sdf(volume, instance, p3, t) +
      vec3f{1, 1, 1} * eval_sdf(volume, instance, p4, t));
}

// Part of this function is inspired by eval_texture from yocto_scene
float eval_volume(const volume<float>& vol, const vec3f& uvw, bool no_interpolation) 
{
  if (vol.empty()) return 0;
  // get coordinates normalized for tiling
  float s = clamp((uvw.x + 1.0f) * 0.5f, 0.0f, 1.0f) * (vol.size().x - 1);
  float t = clamp((uvw.y + 1.0f) * 0.5f, 0.0f, 1.0f) * (vol.size().y - 1);
  float r = clamp((uvw.z + 1.0f) * 0.5f, 0.0f, 1.0f) * (vol.size().z - 1);

  // get volume coordinates and residuals
  auto i  = clamp((int)s, 0, vol.size().x - 1);
  auto j  = clamp((int)t, 0, vol.size().y - 1);
  auto k  = clamp((int)r, 0, vol.size().z - 1);

  auto ii = min(i + 1, vol.size().x - 1), jj = min(j + 1, vol.size().y - 1),
       kk = min(k + 1, vol.size().z - 1);
  float u = s - i, v = t - j, w = r - k;
  

  // nearest-neighbor interpolation
  if (no_interpolation) {
    i = u < 0.5 ? i : min(i + 1, vol.size().x - 1);
    j = v < 0.5 ? j : min(j + 1, vol.size().y - 1);
    k = w < 0.5 ? k : min(k + 1, vol.size().z - 1);
    return vol[{i, j, k}];
  }

  // trilinear interpolation
  return vol[{i, j, k}] * (1 - u) * (1 - v) * (1 - w) +
         vol[{ii, j, k}] * u * (1 - v) * (1 - w) +
         vol[{i, jj, k}] * (1 - u) * v * (1 - w) +
         vol[{i, j, kk}] * (1 - u) * (1 - v) * w +
         vol[{i, jj, kk}] * (1 - u) * v * w + 
         vol[{ii, j, kk}] * u * (1 - v) * w + 
         vol[{ii, jj, k}] * u * v * (1 - w) + 
         vol[{ii, jj, kk}] * u * v * w;
}


}  // namespace yocto