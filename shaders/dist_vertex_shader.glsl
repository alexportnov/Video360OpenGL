#version 330
in vec4 aPosition;
in vec4 aTextureCoord;
out vec2 vTextureCoord;

void main()
{
    gl_Position =  aPosition;
    vTextureCoord = aTextureCoord.xy;   
}
