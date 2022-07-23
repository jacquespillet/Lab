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
    if(color.r < lowerBound.r) color.r = 0;
    if(color.g < lowerBound.g) color.g = 0;
    if(color.b < lowerBound.b) color.b = 0;
    
    if(color.r > upperBound.r) color.r = 0;
    if(color.g > upperBound.g) color.g = 0;
    if(color.b > upperBound.b) color.b = 0;

    imageStore ( textureOut , pixelCoord , vec4(color, 0));
}