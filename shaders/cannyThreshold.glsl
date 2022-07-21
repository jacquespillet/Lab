//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;


#define PI 3.14159265359
uniform float threshold;

void main()
{

    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    imageStore ( textureOut , pixelCoord , vec4(0,0,0, 1));

    vec3 currentColor = imageLoad(textureIn, pixelCoord).rgb;

    float lowThreshold=threshold;
    float highThreshold = lowThreshold * 3;

    if(currentColor.r > highThreshold)
    {
        imageStore(textureOut, pixelCoord, vec4(1,0,0,1));
    }
    else if(lowThreshold < currentColor.r && currentColor.r < highThreshold)
    {
        imageStore(textureOut, pixelCoord, vec4(0,1,0,1));
    }
}