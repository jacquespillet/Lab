#version 440 core
out vec4 fragColor;
  
in vec2 fragUv;

uniform sampler2D textureImage;

// uniform vec2
void main()
{
    vec3 result;
    result = texture(textureImage, fragUv).rgb;
    result = pow(result, vec3(1/2.2));
    fragColor = vec4(result, 1.0f) ;
}
