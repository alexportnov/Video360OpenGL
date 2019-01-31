#version 330
in vec2 vTextureCoord;
uniform sampler2D sTexture;
out vec4 outColor;

void main()
{
    outColor = texture(sTexture, vTextureCoord);
}
