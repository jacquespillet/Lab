//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform sampler2D tex;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec2 uv = vec2(pixelCoord) / vec2(imageSize(textureOut) - vec2(1,1));

    vec3 pixelColor = texture(tex, uv).rgb;
    imageStore ( textureOut , pixelCoord , vec4(pixelColor, 1));
}