#version 440 core
out vec4 fragColor;
  
in vec2 fragUv;

uniform sampler2D textureImage;

// uniform vec2
void main()
{
    vec2 uv = fragUv;
    uv.y = 1 -uv.y;
    
    vec3 result = texture(textureImage, uv).rgb;
    fragColor = vec4(result, 1.0f) ;
}
