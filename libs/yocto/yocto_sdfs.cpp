#include "yocto_sdfs.h"
#include "yocto_scene.h"


namespace yocto {

sdf_result eval_sdf(const scene_data& scene, const instance_data& sdf_instance,
    const vec3f& p) 
{
  const auto* sdf = scene.implicits[sdf_instance.implicit];
  return sdf->f(transform_point(sdf_instance.frame, sdf_instance.frame.o) - transform_point(sdf_instance.frame, p));
}

vec3f eval_sdf_normal(const scene_data& scene,
    const instance_data& sdf_instance, const vec3f& p) 
{
  const float h = yocto::flt_eps;  // replace by an appropriate value
  const vec2f k = vec2f{1, -1};
  
  return normalize(vec3f{1, -1, -1} * eval_sdf(scene, sdf_instance, p + vec3f{1, -1, -1} * h) +
                   vec3f{-1, -1, 1} * eval_sdf(scene, sdf_instance, p + vec3f{-1, -1, 1} * h) +
                   vec3f{-1, 1, -1} * eval_sdf(scene, sdf_instance, p + vec3f{-1, 1, -1} * h) +
                    vec3f{1, 1, 1} * eval_sdf(scene, sdf_instance, p + vec3f{1, 1, 1} * h));
}

}  // namespace yocto