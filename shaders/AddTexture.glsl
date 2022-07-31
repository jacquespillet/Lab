//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureA;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform sampler2D textureB;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec2 uv = vec2(pixelCoord) / vec2(imageSize(textureA) - vec2(1,1));

    vec3 pixelColorA = imageLoad(textureA, pixelCoord).rgb;
    vec3 pixelColorB = texture(textureB, uv).rgb;

    imageStore ( textureA , pixelCoord , vec4(pixelColorA + pixelColorB, 1));
}