//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

layout(std430, binding=0) buffer lutBuffer {
    ivec4 lut[];
};

uniform int equalizeColor;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;
    if(equalizeColor>0)
    {
        ivec3 colorInt = min(ivec3(254), max(ivec3(0), ivec3(color * 255.0f)));

        int lutResultR = lut[colorInt.r].r;
        int lutResultG = lut[colorInt.g].g;
        int lutResultB = lut[colorInt.b].b;

        float lutResultFloatR = float(lutResultR) / 255.0f;
        float lutResultFloatG = float(lutResultG) / 255.0f;
        float lutResultFloatB = float(lutResultB) / 255.0f;

        imageStore ( textureOut , pixelCoord , vec4(lutResultFloatR,lutResultFloatG,lutResultFloatB, 1));
    }
    else
    {
        float grayScaleFloat = (color.r + color.g + color.b) * 0.333333;
        int grayScaleInt = min(254, max(0, int(grayScaleFloat * 255.0f)));
        int lutResult = lut[grayScaleInt].a;
        float lutResultFloat = float(lutResult) / 255.0f;
        imageStore ( textureOut , pixelCoord , vec4(lutResultFloat.xxx, 0));
    }
}