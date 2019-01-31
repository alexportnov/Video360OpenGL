#version 330
uniform mat4 uMVPMatrix;
in vec4 aPosition;
in vec4 aTextureCoord;
out vec2 vTextureCoord;
void main()
{
  vTextureCoord = aTextureCoord.xy;
  gl_Position =  uMVPMatrix*aPosition;
}
