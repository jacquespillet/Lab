//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform float lowerBound;
uniform float upperBound;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec4 color = imageLoad(textureIn, pixelCoord);
    float grayScale = ((color.x + color.y + color.z) / 3.0f);
    
    grayScale = (grayScale - lowerBound) / (upperBound - lowerBound);
    imageStore ( textureOut , pixelCoord , vec4(grayScale.xxx, 0));
}