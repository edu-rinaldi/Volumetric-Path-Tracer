#include "yocto_sdfs.h"
#include "yocto_scene.h"


namespace yocto {

vec3f eval_sdf_normal(const sdf& sdf, const vec3f& p) 
{
  const float h = yocto::flt_eps;  // replace by an appropriate value
  const vec2f k = vec2f{1, -1};
  return normalize(
      vec3f{1, -1, -1} * sdf(p + vec3f{1, -1, -1} * h) +
      vec3f{-1, -1, 1} * sdf(p + vec3f{-1, -1, 1} * h) +
      vec3f{-1, 1, -1} * sdf(p + vec3f{-1, 1, -1} * h) +
      vec3f{1, 1, 1} * sdf(p + vec3f{1, 1, 1} * h));
}

}  // namespace yocto