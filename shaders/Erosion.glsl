//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int size;


layout(std430, binding=0) buffer kernelBuffer {
    float kernel[];
};
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    ivec2 imgSize = imageSize(textureIn);

    int halfSize = int(floor( float(size) /2.0f));
    float inverseSize = 1.0f / float(size * size);
    int x=0;

    bool allPixelsIn=true;
    bool shouldBreak=false;
    for(int xx = pixelCoord.x - halfSize; xx <= pixelCoord.x + halfSize; xx++)
    {
        int y=0;
        for(int yy = pixelCoord.y - halfSize; yy <= pixelCoord.y + halfSize; yy++)
        {
            int flatInx = y * size + x;
            ivec2 samplePos = ivec2(xx, yy);
            if(samplePos.x < 0) samplePos.x=0;
            if(samplePos.y < 0) samplePos.y=0;
            if(samplePos.x > imgSize.x-1) samplePos.x=imgSize.x-1;
            if(samplePos.y > imgSize.y-1) samplePos.y=imgSize.y-1;

            float weight = kernel[flatInx];
            vec3 pixelColor = imageLoad(textureIn, samplePos).rgb;
            float pixelValue = (pixelColor.r + pixelColor.g + pixelColor.b) * 0.33333f;

            if(weight > 0 && pixelValue ==0)
            {
                allPixelsIn=false;
                shouldBreak=true;
                break;
            }
            // weight = 1;
            y++;
        }
        if(shouldBreak)break;
        x++;
    }

    vec3 color = allPixelsIn ? vec3(1) : vec3(0);

    imageStore ( textureOut , pixelCoord , vec4(color, 1));
}