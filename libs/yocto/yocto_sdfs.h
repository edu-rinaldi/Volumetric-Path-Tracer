#ifndef _YOCTO_SDFS_H_
#define _YOCTO_SDFS_H_

#include <vector>
#include <string>

#include "yocto_scene.h"
#include "yocto_math.h"


namespace yocto {

struct sdf_result {
  float  dist;
  int    instance;
  inline operator float() const { return dist; }
};

struct sdf_data {
  virtual sdf_result f(const vec3f& p) const = 0;
};


sdf_result eval_sdf(
    const scene_data& scene, const instance_data& sdf_instance, const vec3f& p);

vec3f eval_sdf_normal(
    const scene_data& scene, const instance_data& sdf_instance, const vec3f& p);

}  // namespace yocto



namespace yocto {
inline float sd_plane(const vec3f& p) { return p.y; }

inline float sd_sphere(const vec3f& p, float s) { return length(p) - s; }

inline float sd_box(const vec3f& p, const vec3f& b) {
  vec3f d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, 0.0));
}

inline float op_union(float d1, float d2) { return min(d1, d2); }

inline float op_subtraction(float d1, float d2) { return max(-d1, d2); }

inline float op_intersection(float d1, float d2) { return max(d1, d2); }

inline float op_smooth_union(float d1, float d2, float k) {
  float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
  return lerp(d2, d1, h) - k * h * (1.0 - h);
}

inline float op_smooth_subtraction(float d1, float d2, float k) {
  float h = clamp(0.5 - 0.5 * (d2 + d1) / k, 0.0, 1.0);
  return lerp(d2, -d1, h) + k * h * (1.0 - h);
}

inline float op_smooth_intersection(float d1, float d2, float k) {
  float h = clamp(0.5 - 0.5 * (d2 - d1) / k, 0.0, 1.0);
  return lerp(d2, d1, h) + k * h * (1.0 - h);
}
}

namespace yocto {

struct sdf_plane : public sdf_data 
{
  virtual sdf_result f(const vec3f& p) const override { return sdf_result{sd_plane(p)}; }
};

struct sdf_sphere : public sdf_data 
{
  float radius = 0;

  virtual sdf_result f(const vec3f& p) const override 
  {
    return sdf_result{sd_sphere(p, radius)};
  }
};

struct sdf_box : public sdf_data 
{
  vec3f whd;

  virtual sdf_result f(const vec3f& p) const override {
    return sdf_result{sd_box(p, whd)};
  }
};

template <uint64_t N>
struct sdf_union : public sdf_data {
  sdf_data* functions[N];

  virtual sdf_result f(const vec3f& p) const override {
    float res = functions[0]->f(p);
    for (int i = 1; i < N; ++i) 
        res = op_subtraction(res, functions[i]->f(p));

    return sdf_result{res};
  }
};

}


#endif
