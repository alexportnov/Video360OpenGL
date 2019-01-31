#version 330
in vec2 vTextureCoord;
out vec4 outColor;
uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;

const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);
const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);
const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);
const vec3 offset = vec3(-0.0625, -0.5, -0.5);

void main()
{
  float y = texture(y_tex, vTextureCoord).r;
  float u = texture(u_tex, vTextureCoord).r;
  float v = texture(v_tex, vTextureCoord).r;
  vec3 yuv = vec3(y,u,v);
  yuv += offset;
  outColor = vec4(dot(yuv, R_cf), dot(yuv, G_cf), dot(yuv, B_cf), 1.0);
}
