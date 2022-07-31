//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;
layout ( binding = 2 , rgba16f ) uniform image2D mask;
layout ( binding = 3 , rgba16f ) uniform image2D sourceTexture;

layout ( local_size_x = 32 , local_size_y = 32) in;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    
    vec3 maskColor = imageLoad(mask, pixelCoord).rgb;
    vec3 sourceColor = imageLoad(sourceTexture, pixelCoord).rgb;
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;

    color = (0.5 * color) + (0.5 * sourceColor) + maskColor * 0.5;
    imageStore ( textureOut , pixelCoord, vec4(color, 1));

}