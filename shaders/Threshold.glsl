//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform vec3 lowerBound;
uniform vec3 upperBound;
uniform int global;
uniform int binary;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;
    vec3 outputColor = (binary>0) ?  vec3(1) : color;
    if(global>0)
    {
        float grayScale = (color.r + color.g + color.b) * 0.333333f;
        if(grayScale < lowerBound.r)
        {
            outputColor = vec3(0,0,0);
        }
        if(grayScale > upperBound.r)
        {
            outputColor = vec3(0,0,0);
        }
    }
    else
    {
        if(color.r < lowerBound.r) outputColor.r = 0;
        if(color.g < lowerBound.g) outputColor.g = 0;
        if(color.b < lowerBound.b) outputColor.b = 0;
        
        if(color.r > upperBound.r) outputColor.r = 0;
        if(color.g > upperBound.g) outputColor.g = 0;
        if(color.b > upperBound.b) outputColor.b = 0;
    }

    imageStore ( textureOut , pixelCoord , vec4(outputColor, 0));
}