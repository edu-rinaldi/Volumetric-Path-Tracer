#ifndef _YOCTO_SDFS_H_
#define _YOCTO_SDFS_H_

#include <vector>
#include <string>
#include <functional>

#include "yocto_scene.h"
#include "yocto_math.h"


namespace yocto {

typedef std::function<op_res(const vec3f&)> sdf;

struct op_res {
  float d;
  int   material;
        operator float() { return d; }
};

vec3f eval_sdf_normal(const sdf& sdf, const vec3f& p);

}  // namespace yocto



namespace yocto {
inline float sd_plane(const vec3f& p) { return p.y; }

inline float sd_sphere(const vec3f& p, float s) { return length(p) - s; }

inline float sd_box(const vec3f& p, const vec3f& b) {
  vec3f d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, 0.0));
}

inline float sd_bbox(vec3f p, const vec3f& b, float e) {
  p       = abs(p) - b;
  vec3f q = abs(p + e) - e;

  return min(min(length(max(vec3f{p.x, q.y, q.z}, 0.0f)) +
                     min(max(p.x, max(q.y, q.z)), 0.0f),
                 length(max(vec3f{q.x, p.y, q.z}, 0.0f)) +
                     min(max(q.x, max(p.y, q.z)), 0.0f)),
      length(max(vec3f{q.x, q.y, p.z}, 0.0f)) +
          min(max(q.x, max(q.y, p.z)), 0.0f));
}

inline float sd_torus(const vec3f& p, const vec2f& t) {
  return length(vec2f{length(vec2f{p.x, p.z}) - t.x, p.y}) - t.y;
}

// vertical
inline float sd_cone(const vec3f& p, const vec2f& c, float h) {
  vec2f q = h * vec2f{c.x, -c.y} / c.y;
  vec2f w = vec2f{length(vec2f{p.x, p.z}), p.y};

  vec2f  a = w - q * clamp(dot(w, q) / dot(q, q), 0.0f, 1.0f);
  vec2f b = w - q * vec2f{clamp(w.x / q.x, 0.0f, 1.0f), 1.0};
  float k = sign(q.y);
  float d = min(dot(a, a), dot(b, b));
  float s = max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));
  return sqrt(d) * sign(s);
}

inline float sd_capped_cone(const vec3f& p, float h, float r1, float r2) {
  vec2f q = vec2f{length(vec2f{p.x, p.z}), p.y};

  vec2f  k1 = vec2f{r2, h};
  vec2f  k2 = vec2f{r2 - r1, 2.0f * h};
  vec2f  ca = vec2f{q.x - min(q.x, (q.y < 0.0) ? r1 : r2), abs(q.y) - h};
  vec2f  cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot(k2, k2), 0.0f, 1.0f);
  float s  = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
  return s * sqrt(min(dot(ca, ca), dot(cb, cb)));
}

// c is the sin/cos of the desired cone angle
inline float sd_solid_angle(const vec3f& pos, const vec2f& c, float ra) {
  vec2f p = vec2f{length(vec2f{pos.x, pos.z}), pos.y};
  float l = length(p) - ra;
  float m = length(p - c * clamp(dot(p, c), 0.0f, ra));
  return max(l, m * sign(c.y * p.x - c.x * p.y));
}

inline float         op_union(float d1, float d2) { return min(d1, d2); }
inline const op_res& op_union(const op_res& r1, const op_res& r2) {
  return r1.d < r2.d ? r1 : r2;
}

inline float op_subtraction(float d1, float d2) { return max(-d1, d2); }
inline op_res op_subtraction(const op_res& r1, const op_res& r2) {
  return -r1.d > r2.d ? op_res{-r1.d, r1.material} : r2;
}

inline float op_intersection(float d1, float d2) { return max(d1, d2); }
inline const op_res& op_intersection(const op_res& r1, const op_res& r2) {
  return r1.d > r2.d ? r1 : r2;
}

}  // namespace yocto


#endif
