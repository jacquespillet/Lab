//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform vec3 lowerBound;
uniform vec3 upperBound;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;
    color = (color - lowerBound) / (upperBound - lowerBound);
    imageStore ( textureOut , pixelCoord , vec4(color.xyz, 0));
}