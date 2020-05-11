#version 450 core

in flat vec3 vPos;
in vec3 vColor;

uniform vec3 u_viewpos;
uniform mat4 u_invProj;
uniform mat4 u_invView;
uniform vec2 u_viewportSize;
uniform float sunAngle;
uniform mat4 u_viewProj;

out vec4 fragColor;

// TODO: get position of ray hit to write to depth buffer (gl_FragDepth...)
bool AABBIntersect(vec3 ro, vec3 rd, vec3 minV, vec3 maxV)
{
  // inverse of ray direction
  vec3 invR = 1.0 / rd;

  // 
  vec3 tbot = (minV - ro) * invR;
  vec3 ttop = (maxV - ro) * invR;

  vec3 tmin = min(ttop, tbot);
  vec3 tmax = max(ttop, tbot);

  vec2 t = max(tmin.xx, tmin.yz);

  float t0 = max(t.x, t.y);
  t = min(tmax.xx, tmax.yz);
  float t1 = min(t.x, t.y);

  //return vec2(t0, t1); 
  if (t0 <= t1) { return true; } else { return false; }
}


struct Box
{
  vec3 radius;
  vec3 invRadius;
  vec3 center;
};
struct Ray
{
  vec3 origin;
  vec3 dir;
};

float maxv(vec3 v) { return max(max(v.x, v.y), v.z); }
bool ourIntersectBox(Box box, Ray ray, out float distance, out vec3 normal)
{
  ray.origin = ray.origin - box.center;

  vec3 sgn = -sign(ray.dir);
  // Distance to plane
  vec3 d = box.radius * sgn - ray.origin;
  d *= 1.0 / ray.dir;

  #define TEST(U, VW) (d.U >= 0.0) && \
    all(lessThan(abs(ray.origin.VW + ray.dir.VW * d.U), box.radius.VW))
  bvec3 test = bvec3(TEST(x, yz), TEST(y, zx), TEST(z, xy));
  sgn = test.x ? vec3(sgn.x,0,0) : (test.y ? vec3(0,sgn.y,0) : vec3(0,0,test.z ? sgn.z:0));
  #undef TEST

  normal = sgn;
  distance = (sgn.x != 0) ? d.x : ((sgn.y != 0) ? d.y : d.z);
  return (sgn.x != 0) || (sgn.y != 0) || (sgn.z != 0);
}


// this function actually works wtf
vec3 WorldPosFromDepth(float depth)
{
  float z = depth * 2.0 - 1.0;
  vec2 TexCoord = gl_FragCoord.xy / u_viewportSize;
  vec4 clipSpacePosition = vec4(TexCoord * 2.0 - 1.0, z, 1.0);
  vec4 viewSpacePosition = u_invProj * clipSpacePosition;

  // Perspective division
  viewSpacePosition /= viewSpacePosition.w;

  vec4 worldSpacePosition = u_invView * viewSpacePosition;

  return worldSpacePosition.xyz;
}


void main()
{
  const bool debug = false; // display splat points

  vec3 ro = WorldPosFromDepth(0);
  vec3 rd = normalize(ro - u_viewpos);

  fragColor.a = 1.0;
#if 0 // use slab method (no normals, depth, etc)
  bool intersect = AABBIntersect(ro, rd, vPos - 0.5, vPos + 0.5);
  rd = rd * .5 + .5;
  if (intersect == true)
    fragColor.xyz = vColor * max(sunAngle, .01);
  else if (!debug)
    discard;
  else
    fragColor.xyz = vec3(1);
#else // use "efficient" method (yet to be benchmarked)
  Ray ray;
  Box box;
  ray.origin = ro;
  ray.dir = rd;
  box.center = vPos;
  box.radius = vec3(0.5);
  box.invRadius = vec3(1.0) / box.radius;
  float dist;
  vec3 normal;
  bool intersect = ourIntersectBox(box, ray, dist, normal);
  if (intersect == true)
  {
    float darken = normal.x!=0 ? .1 : (normal.z!=0 ? .2 : 0); // mild darkening based on normal
    fragColor.xyz = vColor * max(sunAngle, .01);
    fragColor.xyz *= 1 - darken;

    // do some thing to figure out the proper depth (doens't really matter)
    //vec4 v_clip_coord = u_viewProj * vec4(vPos, 1.0);
    //float f_ndc_depth = v_clip_coord.z / v_clip_coord.w;
    //gl_FragDepth = (gl_DepthRange.far - gl_DepthRange.near) * 0.5 * f_ndc_depth + (gl_DepthRange.far + gl_DepthRange.near) * 0.5;
  }
  else if (!debug)
    discard;
  else
    fragColor.xyz = vec3(1);
#endif
}