//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int levels;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;

    // ivec3 colorInt = ivec3(color * 255.0f);
    ivec3 colorInt = ivec3(color * float(levels));
    vec3 finalColor = vec3(colorInt) / float(levels);
    
    imageStore ( textureOut , pixelCoord , vec4(finalColor, 1));
}