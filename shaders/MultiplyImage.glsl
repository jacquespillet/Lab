//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform sampler2D textureToMultiply;
uniform float multiplier;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;
    
    vec2 uv = vec2(pixelCoord) / vec2(imageSize(textureIn));
    vec3 textureColor = multiplier * texture(textureToMultiply, uv).rgb;
    color *= textureColor;
    // color = 1 - color;
    imageStore ( textureOut , pixelCoord , vec4(color, 1));
}