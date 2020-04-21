#version 410 core

uniform vec2 u_resolution; // viewport resolution (in pixels)
uniform float u_time; // in seconds
uniform float u_xrot;
uniform float u_yrot;
uniform float u_zrot;
uniform sampler1D texFFT;

layout(location = 0) out vec4 out_color; // out_color must be written in order to see anything

const int   MAX_STEP = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON  = 0.0001;

float f(float x, float z)
{
  return 2.0;
}


// transformations
mat4 rotateX(float theta)
{
  float c = cos(theta);
  float s = sin(theta);

  return mat4(
    vec4(1, 0, 0, 0),
    vec4(0, c, s, 0),
    vec4(0, -s, c, 0),
    vec4(0, 0, 0, 1)
  );  
}

// transformations
mat4 rotateY(float theta)
{
  float c = cos(theta);
  float s = sin(theta);

  return mat4(
    vec4(c, 0, s, 0),
    vec4(0, 1, 0, 0),
    vec4(-s, 0, c, 0),
    vec4(0, 0, 0, 1)
  );  
}

// transformations
mat4 rotateZ(float theta)
{
  float c = cos(theta);
  float s = sin(theta);

  return mat4(
    vec4(c, s, 0, 0),
    vec4(-s, c, 0, 0),
    vec4(0, 0, 1, 0),
    vec4(0, 0, 0, 1)
  );  
}

float tHeight(vec2 p)
{
  return smoothstep(0.0, 200.0, p.y*20000)*5;
}

float torusSDF( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  float d1 = length(p)-0.4;
  float d2 = tHeight(p.xy)*0.45;
  return d1 + (d2/20);
}

vec3 transformScene(vec3 p)
{
    //p = (inverse(rotateX(u_time*3)) * vec4(p, 1.0)).xyz;
    p = (inverse(rotateX(u_xrot)) *
         inverse(rotateY(u_yrot)) *
         inverse(rotateZ(u_zrot)) * vec4(p, 1.0)).xyz;
    return p;
}

// Standard distance function for the whole scene!
float sceneSDF(vec3 p)
{
  p = transformScene(p);
  //p = p * vec3(1.0, sin(u_time),1.0);
  return torusSDF(p, vec2(0.3, 0.1));
}

vec3 estNormal(vec3 p)
{
  return normalize(vec3(
    sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
    sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
    sceneSDF(vec3(p.x, p.y, p.z + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
  ));
}

vec3 rayDirection(float fov, vec2 size, vec2 fragCoord)
{
  vec2 xy = fragCoord - size / 2.0;
  float z = size.y  / tan(radians(fov) / 2.0);
  return normalize(vec3(xy, -z));
}

// Raymarcher!
float march(vec3 eye, vec3 dir, float start, float end)
{
  float depth = start;
  for (int i=0; i<MAX_STEP; i++) {
    float dist = sceneSDF(eye + depth * dir);
    if (dist < 0.0001) {
      return depth; // we're inside the scene
    }
    depth+=dist;
    if (depth >= end) {
      return end; // gone too far! 
    }
  }
}

// Does something related to phong for each light that we want
vec3 phongLightContrib(vec3 kd, vec3 ks, float alpha, vec3 p, vec3 eye, vec3 lightPos, vec3 lightInt)
{
  vec3 N = estNormal(p);
  vec3 L = normalize(lightPos - p);
  vec3 V = normalize(eye - p);
  vec3 R = normalize(reflect(-L, N));

  float dotLN = dot(L, N);
  float dotRV = dot(R, V);

  if (dotLN < 0.0) {return vec3(0.0);}
  if (dotRV < 0.0) {return lightInt * (kd * dotLN);}
  return lightInt * (kd * dotLN + ks * pow(dotRV, alpha));
}

vec3 phong(vec3 ka, vec3 kd, vec3 ks, float alpha, vec3 p, vec3 eye)
{
  const vec3 ambientLux = vec3(0.3);
  vec3 colour = ambientLux * ka;
  
  vec3 light1Pos = vec3(-1.0, -1.2, 0.7);
  vec3 light1Int = vec3(0.2);
  
  vec3 light2Pos = vec3(0.8, 0.8, 1.2);
  vec3 light2Int = vec3(0.55);
  
  colour += phongLightContrib(kd, ks, alpha, p, eye, light1Pos, light1Int);
  colour += phongLightContrib(kd, ks, alpha, p, eye, light2Pos, light2Int);
  return colour;
}

void main(void)
{
  vec3 dir = rayDirection(45.0, u_resolution.xy, gl_FragCoord.xy);
  vec3 eye = vec3(0.0,0.0,5.0);
  float dist = march(eye, dir, MIN_DIST, MAX_DIST);
  
  if (dist > MAX_DIST - EPSILON) {out_color = vec4(0.1); return;}
  
  vec3 p = eye + dist * dir;
  vec3 colour;
  
  vec3 Ka = vec3(0.0, 0.729, 0.745)*texture(texFFT, 0.123).r*1000; // ambient  reflection constant
  vec3 Kd = vec3(0.0, 0.467, 0.745);                               // diffuse  reflection constant
  vec3 Ks = vec3(0.0, 0.0, 0.0);                                   // specular reflection constant
  float shininess = 1.0;
  colour = phong(Ka, Kd, Ks, shininess, p, eye);
  
  if (transformScene(p).y< 0) {
    vec3 Ka = vec3(0.2);            // ambient  reflection constant
    vec3 Kd = vec3(0.95, 0.2, 0.1); // diffuse  reflection constant
    vec3 Ks = vec3(0.95, 0.2, 0.1); // specular reflection constant
    float shininess = 300.0;
    colour = phong(Ka, Kd, Ks, shininess, p, eye);
  }
  
  out_color = vec4(colour, 1.0);
}
