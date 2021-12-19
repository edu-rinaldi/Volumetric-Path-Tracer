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
  int    subinstance;
  inline operator float() const { return dist; }
};

struct sdf_data {
  virtual float f(const vec3f& p) const = 0;
};


sdf_result eval_sdf(
    const scene_data& scene, int instance, const vec3f& p);

vec3f eval_sdf_normal(
    const scene_data& scene, int instance, const vec3f& p);

}  // namespace yocto



namespace yocto {
inline float sd_plane(const vec3f& p) { return p.y; }

inline float sd_sphere(const vec3f& p, float s) { return length(p) - s; }

inline float sd_box(const vec3f& p, const vec3f& b) {
  vec3f d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, 0.0));
}

inline float op_union(float d1, float d2) { return min(d1, d2); }

inline sdf_result op_union(const sdf_result& d1, const sdf_result& d2) 
{
  return d1.dist < d2.dist ? d1 : d2;
}

inline float op_subtraction(float d1, float d2) { return max(-d1, d2); }

inline sdf_result op_subtraction(const sdf_result& d1, const sdf_result& d2) {
  return -d1.dist > d2.dist ? sdf_result{-d1.dist, d1.instance, d1.subinstance}
                            : d2;
}

inline float op_intersection(float d1, float d2) { return max(d1, d2); }

inline sdf_result op_intersection(const sdf_result& d1, const sdf_result& d2) {
  return d1.dist > d2.dist ? d1 : d2;
}

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
}  // namespace yocto

namespace yocto {

struct sdf_plane : public sdf_data 
{
  virtual float f(const vec3f& p) const override {
    return sd_plane(p);
  }
};

struct sdf_sphere : public sdf_data 
{
  float radius = 0;

  virtual float f(const vec3f& p) const override 
  {
    return sd_sphere(p, radius);
  }
};

struct sdf_box : public sdf_data 
{
  vec3f whd;

  virtual float f(const vec3f& p) const override {
    return sd_box(p, whd);
  }
};

} // namespace yocto


#endif
