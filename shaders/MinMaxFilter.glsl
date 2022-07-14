//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int size;
uniform int doMin;


void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    ivec2 imgSize = imageSize(textureIn);

    int halfSize = size /2 ;
    vec3 color = doMin > 0 ?  vec3(1) : vec3(0);

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
            if(doMin>0)color = min(sampleColor, color);
            else color = max(sampleColor, color);
        }
    }
    imageStore ( textureOut , pixelCoord , vec4(color, 0));
}