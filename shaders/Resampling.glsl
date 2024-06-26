//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int pixelSize;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );

    float x = int(pixelCoord.x) % pixelSize;
    float y = int(pixelCoord.y) % pixelSize;
    x = floor(pixelSize / 2.0) - x;
    y = floor(pixelSize / 2.0) - y;
    x = pixelCoord.x + x;
    y = pixelCoord.y + y;    
    vec3 finalColor = imageLoad(textureIn, ivec2(x, y)).rgb;

    
    imageStore ( textureOut , pixelCoord , vec4(finalColor, 1));
}