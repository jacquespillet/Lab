//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform vec3 color;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 pixelColor = imageLoad(textureIn, pixelCoord).rgb;
    pixelColor += color;
    // color = 1 - color;
    imageStore ( textureOut , pixelCoord , vec4(pixelColor, 1));
}