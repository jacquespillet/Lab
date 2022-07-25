//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int size;
uniform float a;
uniform float b;
void main()
{
    ivec2 imgSize = imageSize(textureIn);
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;
    float grayScale = (color.r + color.g + color.b) * 0.33333f;

    int halfSize = size /2 ;
    float mean=0;
    float count=0;
    for(int xx = pixelCoord.x - halfSize; xx <= pixelCoord.x + halfSize; xx++)
    {
        for(int yy = pixelCoord.y - halfSize; yy <= pixelCoord.y + halfSize; yy++)
        {
            
            ivec2 samplePos = ivec2(xx, yy);
            if(samplePos.x < 0) samplePos.x=0;
            if(samplePos.y < 0) samplePos.y=0;
            if(samplePos.x > imgSize.x-1) samplePos.x=imgSize.x-1;
            if(samplePos.y > imgSize.y-1) samplePos.y=imgSize.y-1;

            vec3 sampleColor = imageLoad(textureIn, samplePos).rgb;
            float sampleGrayScale = (sampleColor.r + sampleColor.g + sampleColor.b) * 0.3333f;
            mean += sampleGrayScale;
            count +=1;
        }
    }
    mean /= count;
    float variance=0;
    for(int xx = pixelCoord.x - halfSize; xx <= pixelCoord.x + halfSize; xx++)
    {
        for(int yy = pixelCoord.y - halfSize; yy <= pixelCoord.y + halfSize; yy++)
        {
            
            ivec2 samplePos = ivec2(xx, yy);
            if(samplePos.x < 0) samplePos.x=0;
            if(samplePos.y < 0) samplePos.y=0;
            if(samplePos.x > imgSize.x-1) samplePos.x=imgSize.x-1;
            if(samplePos.y > imgSize.y-1) samplePos.y=imgSize.y-1;

            vec3 sampleColor = imageLoad(textureIn, samplePos).rgb;
            float sampleGrayScale = (sampleColor.r + sampleColor.g + sampleColor.b) * 0.3333f;
            variance += (sampleGrayScale - mean) * (sampleGrayScale - mean);
        }
    }

    float stdDeviation = sqrt(variance);
    if(abs(grayScale - mean) < stdDeviation)
    {
        
    }
    else
    {
        color = vec3(0,0,0);
    }

    imageStore ( textureOut , pixelCoord , vec4(color, 0));
}