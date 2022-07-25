//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;
layout ( binding = 2 , rgba16f ) uniform image2D textureToAdd;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform float multiplier;
uniform ivec2 offset;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    if(pixelCoord.x < imageSize(textureToAdd).x && pixelCoord.y < imageSize(textureToAdd).y)
    {
        vec3 color = imageLoad(textureIn, pixelCoord).rgb;
        
        vec3 textureColor = multiplier * imageLoad(textureToAdd, pixelCoord).rgb;
        color += textureColor;
        // color = 1 - color;
        imageStore ( textureOut , pixelCoord + offset , vec4(color, 0));
    }
}