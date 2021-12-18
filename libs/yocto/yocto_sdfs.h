#ifndef _YOCTO_SDFS_H_
#define _YOCTO_SDFS_H_

#include <vector>
#include <string>

#include "yocto_scene.h"
#include "yocto_math.h"


namespace yocto {

struct sdf_result {
  float  dist;
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
}

namespace yocto {

struct sdf_plane : public sdf_data {
  virtual sdf_result f(const vec3f& p) const override {
    return sdf_result{sd_plane(p)};
  }
};

struct sdf_sphere : public sdf_data 
{
  float radius = 0;

  virtual sdf_result f(const vec3f& p) const override {
    return sdf_result{ max(sd_sphere(p, radius), -sd_sphere(p, radius-0.0399999f))};
  }
};

struct sdf_box : public sdf_data 
{
  float width = 0;
  float height = 0;
  float depth = 0;

  virtual sdf_result f(const vec3f& p) const override {
    return sdf_result { sd_box(p, {width, height, depth}) };
  }
};


}


#endif
