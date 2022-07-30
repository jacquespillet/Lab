//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform sampler2D gradientTexture;
uniform mat2 transform;
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 pixelColor = imageLoad(textureIn, pixelCoord).rgb;
    ivec2 imgSize = imageSize(textureIn);
    vec2 imgHalfSize = vec2(imgSize)/2;

    vec2 pixelCoordCartesian = vec2(pixelCoord) - imgHalfSize;
    pixelCoordCartesian = transform * pixelCoordCartesian;
    pixelCoordCartesian+=imgHalfSize;

    float t = float(pixelCoordCartesian.x) / float(imgSize.x);
    vec3 gradientColor = texture(gradientTexture, vec2(t, 0)).rgb;

    pixelColor += gradientColor;
    // color = 1 - color;
    imageStore ( textureOut , pixelCoord , vec4(pixelColor, 1));
}