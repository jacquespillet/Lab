//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform float saturation;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    imageStore ( textureOut , pixelCoord , vec4(0,0,0,1));
}