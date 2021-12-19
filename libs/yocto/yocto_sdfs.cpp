#include "yocto_sdfs.h"
#include "yocto_scene.h"


namespace yocto {

sdf_result eval_sdf(const scene_data& scene, int instance, const vec3f& p) 
{
  const auto& inst = scene.implicits_instances[instance];

  const auto&    sdf   = scene.implicits[inst.implicits[0]];
  const frame3f& frame = inst.frames[0];

  sdf_result res = {
      sdf->f(transform_point(frame, frame.o) - transform_point(frame, p)),
      instance, 0};

  if (inst.type == sdf_instance_type::primitive)
    return res;

  for (int i = 1; i < inst.implicits.size(); ++i) 
  {
    const auto&    sdf   = scene.implicits[inst.implicits[i]];
    const frame3f& frame = inst.frames[i];
    sdf_result     new_res = {
        sdf->f(transform_point(frame, frame.o) - transform_point(frame, p)), instance, i};
    switch (inst.type) {
      case sdf_instance_type::union_op: res = op_union(res, new_res); break;
      case sdf_instance_type::intersection_op: res = op_intersection(res, new_res); break;
      case sdf_instance_type::subtraction_op: res = op_subtraction(res, new_res); break;
    }
  }
  return res;
}

vec3f eval_sdf_normal(const scene_data& scene, int instance, const vec3f& p) 
{
  const float h = yocto::flt_eps;  // replace by an appropriate value
  const vec2f k = vec2f{1, -1};
  return normalize(
      vec3f{1, -1, -1} * eval_sdf(scene, instance, p + vec3f{1, -1, -1} * h) +
      vec3f{-1, -1, 1} * eval_sdf(scene, instance, p + vec3f{-1, -1, 1} * h) +
      vec3f{-1, 1, -1} * eval_sdf(scene, instance, p + vec3f{-1, 1, -1} * h) +
      vec3f{1, 1, 1} * eval_sdf(scene, instance, p + vec3f{1, 1, 1} * h));
}

}  // namespace yocto