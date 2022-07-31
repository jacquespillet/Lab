//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform sampler2D sourceTexture;
uniform sampler2D destTexture;
uniform sampler2D maskTexture;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec2 uv = vec2(pixelCoord) / vec2(imageSize(textureOut) - vec2(1,1));

    vec3 targetColor = texture(destTexture,  uv).rgb;
    float mask       = texture(maskTexture,  uv).r;
    vec3 sourceColor = texture(sourceTexture,uv).rgb;

    vec3 color = imageLoad(textureOut, pixelCoord).rgb;
    color += mask * sourceColor + (1-mask) * targetColor;

    imageStore(textureOut, pixelCoord, vec4(color, 1));
}