#include "yocto_sdfs.h"
#include "yocto_scene.h"

namespace yocto {

float eval_sdf(const volume<float>& volume, const volume_instance& instance,
    const vec3f& p, float t) {
  
  vec3f grid_res = {volume.whd.x, volume.whd.y, volume.whd.z};
  const auto& origin   = instance.frame.o;
 
  auto  bbox_max      = origin + (volume.res * grid_res) * instance.scalef;
  auto  bbox_size     = (bbox_max - origin);
  float bbox_dist     = sd_box(p - (origin + (bbox_size/2.f)), (bbox_size/2.f));
  if (bbox_dist < flt_eps * t ) {
    vec3f uvw = (p - origin);
    uvw       = uvw * 2.f / (bbox_size) - 1;
    float sdf = eval_volume(volume, uvw) * instance.scalef;
    
    return sdf;
  }

  return bbox_dist;
}

//float eval_sdf(const volume<float>& volume, const volume_instance& instance,
//    const vec3f& p, float t) {
//  auto grid_res = vec3f{360, 95, 337};
//  auto origin   = instance.frame.o;
//  // auto bbox_max = vec3f{0.0627788, 0.188996, 0.0607146};
//  // auto bbox_max            = origin + 0.002f * grid_res;
//  auto  bbox_max  = origin + grid_res;
//  auto  bbox_size = (bbox_max - origin);
//  float bbox_dist = sd_box(p - (origin + (bbox_size / 2.f)), (bbox_size / 2.f));
//  if (bbox_dist < flt_eps * t) {
//    vec3f uvw = (p - origin);
//    uvw       = uvw * 2.f / (bbox_size) - 1;
//    
//    float sdf = eval_volume(volume, uvw);
//
//    return sdf;
//  }
//
//  return bbox_dist;
//}


vec3f eval_sdf_normal(const sdf& sdf, const vec3f& p) {
  const float h = yocto::flt_eps;  // replace by an appropriate value
  return normalize(vec3f{1, -1, -1} * sdf(p + vec3f{1, -1, -1} * h) +
                   vec3f{-1, -1, 1} * sdf(p + vec3f{-1, -1, 1} * h) +
                   vec3f{-1, 1, -1} * sdf(p + vec3f{-1, 1, -1} * h) +
                   vec3f{1, 1, 1} * sdf(p + vec3f{1, 1, 1} * h));
}

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

// Evaluates a color image at a point `uv`.
float eval_volume(
    const volume<float>& vol, const vec3f& uvw, bool no_interpolation) {
  if (vol.empty()) return 0;

  // get coordinates normalized for tiling
  float s = clamp((uvw.x + 1.0f) * 0.5f, 0.0f, 1.0f) * (vol.size().x - 1);
  float t = clamp((uvw.y + 1.0f) * 0.5f, 0.0f, 1.0f) * (vol.size().y - 1);
  float r = clamp((uvw.z + 1.0f) * 0.5f, 0.0f, 1.0f) * (vol.size().z - 1);

  // get image coordinates and residuals
  auto i  = clamp((int)s, 0, vol.size().x - 1);
  auto j  = clamp((int)t, 0, vol.size().y - 1);
  auto k  = clamp((int)r, 0, vol.size().z - 1);
  /*auto ii = i + 1 < vol.size().x ? i+1 : i, jj = j + 1 < vol.size().y ? j+1 : j,
       kk = (k + 1) < vol.size().z ? k+1 : k;*/
  /*auto ii = (i + 1) % (vol.size().x - 1), jj = (j + 1) % (vol.size().y - 1),
       kk = (k + 1) % (vol.size().z - 1);*/
  auto ii = min(i + 1, vol.size().x - 1), jj = min(j + 1, vol.size().y - 1),
       kk = min(k + 1, vol.size().z - 1);
  float u = s - i, v = t - j, w = r - k;
  

  // nearest-neighbor interpolation
  if (no_interpolation) {
    i = u < 0.5 ? i : min(i + 1, vol.size().x - 1);
    j = v < 0.5 ? j : min(j + 1, vol.size().y - 1);
    k = w < 0.5 ? k : min(k + 1, vol.size().z - 1);
    return lookup_volume(vol, {i, j, k});
  }

  // trilinear interpolation
  return lookup_volume(vol, {i, j, k}) * (1 - u) * (1 - v) * (1 - w) +
         lookup_volume(vol, {ii, j, k}) * u * (1 - v) * (1 - w) +
         lookup_volume(vol, {i, jj, k}) * (1 - u) * v * (1 - w) +
         lookup_volume(vol, {i, j, kk}) * (1 - u) * (1 - v) * w +
         lookup_volume(vol, {i, jj, kk}) * (1 - u) * v * w +
         lookup_volume(vol, {ii, j, kk}) * u * (1 - v) * w +
         lookup_volume(vol, {ii, jj, k}) * u * v * (1 - w) +
         lookup_volume(vol, {ii, jj, kk}) * u * v * w;
}

}  // namespace yocto