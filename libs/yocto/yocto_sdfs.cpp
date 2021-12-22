#include "yocto_sdfs.h"

#include "yocto_scene.h"

namespace yocto {

vec3f eval_sdf_normal(const sdf& sdf, const vec3f& p) {
  const float h = yocto::flt_eps;  // replace by an appropriate value
  return normalize(vec3f{1, -1, -1} * sdf(p + vec3f{1, -1, -1} * h) +
                   vec3f{-1, -1, 1} * sdf(p + vec3f{-1, -1, 1} * h) +
                   vec3f{-1, 1, -1} * sdf(p + vec3f{-1, 1, -1} * h) +
                   vec3f{1, 1, 1} * sdf(p + vec3f{1, 1, 1} * h));
}

// Evaluates a color image at a point `uv`.
float eval_volume(
    const volume<float>& vol, const vec3f& uvw, bool no_interpolation) {
  if (vol.empty()) return 0;

  // get coordinates normalized for tiling
  auto s = clamp((uvw.x + 1.0f) * 0.5f, 0.0f, 1.0f) * vol.size().x;
  auto t = clamp((uvw.y + 1.0f) * 0.5f, 0.0f, 1.0f) * vol.size().y;
  auto r = clamp((uvw.z + 1.0f) * 0.5f, 0.0f, 1.0f) * vol.size().z;

  // get image coordinates and residuals
  auto i  = clamp((int)s, 0, vol.size().x - 1);
  auto j  = clamp((int)t, 0, vol.size().y - 1);
  auto k  = clamp((int)r, 0, vol.size().z - 1);
  auto ii = (i + 1) % vol.size().x, jj = (j + 1) % vol.size().y,
       kk = (k + 1) % vol.size().z;
  auto u = s - i, v = t - j, w = r - k;

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