#ifndef _YOCTO_SDFS_H_
#define _YOCTO_SDFS_H_

#include <functional>
#include <string>
#include <vector>

#include "yocto_math.h"
#include "yocto_scene.h"

namespace yocto {

// sdf as pointer to a function
typedef std::function<float(const vec3f&)> sdf;

struct sdf_result {
  float result = flt_max;
  int   instance = invalidid;
  int   sdf      = invalidid;
};

sdf_result eval_sdf_scene(const scene_data& scene, const vec3f& p, float t);

float eval_sdf(const volume<float>& volume, const volume_instance& instance, const vec3f& p, float t);
vec3f eval_sdf_normal(const scene_data& scene, const vec3f& p, float t);
vec3f eval_sdf_normal(const sdf_data& sdf, const vec3f& p, float t);
vec3f eval_sdf_normal(const volume<float>& volume, const volume_instance& instance, const vec3f& p, float t);

inline float lookup_volume(const volume<float>& vol,const vec3i& ijk) {
  return vol[ijk];
}

float eval_volume(
    const volume<float>& vol, const vec3f& uvw, bool no_interpolation = false);


enum struct sdf_type { bbox, box, capped_cone, plane, sphere, torus };

inline float sd_plane(const vec3f& p) { return p.y; }
inline float sd_sphere(const vec3f& p, float s) { return length(p) - s; }
inline float sd_box(const vec3f& p, const vec3f& b) 
{
  vec3f d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, 0.0));
}

inline float sd_bbox(vec3f p, const vec3f& b, float e) 
{
  p       = abs(p) - b;
  vec3f q = abs(p + e) - e;

  return min(min(length(max(vec3f{p.x, q.y, q.z}, 0.0f)) +
                     min(max(p.x, max(q.y, q.z)), 0.0f),
                 length(max(vec3f{q.x, p.y, q.z}, 0.0f)) +
                     min(max(q.x, max(p.y, q.z)), 0.0f)),
      length(max(vec3f{q.x, q.y, p.z}, 0.0f)) +
          min(max(q.x, max(q.y, p.z)), 0.0f));
}

inline float sd_torus(const vec3f& p, float r1, float r2) 
{
  // r1 is outer radius, r2 is inner radius
  return length(vec2f{length(vec2f{p.x, p.z}) - r1, p.y}) - r2;
}

inline float sd_capped_cone(const vec3f& p, float h, float r1, float r2) 
{
  vec2f q = vec2f{length(vec2f{p.x, p.z}), p.y};

  vec2f k1 = vec2f{r2, h};
  vec2f k2 = vec2f{r2 - r1, 2.0f * h};
  vec2f ca = vec2f{q.x - min(q.x, (q.y < 0.0) ? r1 : r2), abs(q.y) - h};
  vec2f cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot(k2, k2), 0.0f, 1.0f);
  float s  = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
  return s * sqrt(min(dot(ca, ca), dot(cb, cb)));
}

inline float         op_union(float d1, float d2) { return min(d1, d2); }
inline const sdf_result& op_union(const sdf_result& r1, const sdf_result& r2) {
  return r1.result < r2.result ? r1 : r2;
}

inline float  op_subtraction(float d1, float d2) { return max(-d1, d2); }
inline sdf_result op_subtraction(const sdf_result& r1, const sdf_result& r2) {
  return -r1.result > r2.result ? sdf_result{-r1.result, r1.instance, r1.sdf} : r2;
}

inline float         op_intersection(float d1, float d2) { return max(d1, d2); }
inline const sdf_result& op_intersection(const sdf_result& r1, const sdf_result& r2) {
  return r1.result > r2.result ? r1 : r2;
}

}  // namespace yocto

#endif