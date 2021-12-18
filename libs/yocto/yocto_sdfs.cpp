#include "yocto_sdfs.h"
#include "yocto_scene.h"


namespace yocto {

sdf_result eval_sdf(const scene_data& scene, const instance_data& sdf_instance,
    const vec3f& p) 
{
  const auto* sdf = scene.implicits[sdf_instance.implicit];
  return sdf->f(sdf_instance.frame.o - transform_point(sdf_instance.frame, p));
}

vec3f eval_sdf_normal(const scene_data& scene,
    const instance_data& sdf_instance, const vec3f& p) 
{
  vec3f n = zero3f;
  for (int i = 0; i < 4; ++i) {
    vec3f e = 0.5773 * (2.0 * vec3f{(float)(((i + 3) >> 1) & 1),
                                  (float)((i >> 1) & 1), (float)(i & 1)} -
                           1.0f);
    n += e * eval_sdf(scene, sdf_instance, p + yocto::flt_eps * e).dist;
    // if( n.x+n.y+n.z>100.0 ) break;
  }
  return normalize(n);
}

}  // namespace yocto