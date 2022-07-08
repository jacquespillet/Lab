//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

layout(std430, binding=0) buffer histogramBuffer {
    ivec4 histogram[];
};

layout(std430, binding=1) buffer boundsBuffer {
    // ivec4 minBound;
    ivec4 maxBound;
};

uniform int histogramR;
uniform int histogramG;
uniform int histogramB;
uniform int histogramGray;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    if(pixelCoord.x > 255 && pixelCoord.y > 255) return;
    
    int x = pixelCoord.x;
    int y = pixelCoord.y;
    
    imageStore ( textureOut , pixelCoord , vec4(0,0,0, 1));
    
    float normalizedY = 1.0f - float(y) / 255.0f;
    uint maxValue = max(maxBound.x, max(maxBound.y, max(maxBound.z, maxBound.w)));
    float yHistogramSpace = normalizedY * float(maxValue);
    
    if(histogramR > 0 && histogram[x].x != 0 && histogram[x].x > yHistogramSpace)
    {
        imageStore ( textureOut , pixelCoord , vec4(1,0,0, 1));
    }
    
    if(histogramG > 0 && histogram[x].y != 0 && histogram[x].y > yHistogramSpace)
    {
        imageStore ( textureOut , pixelCoord , vec4(0,1,0, 1));
    }
    
    if(histogramB > 0 && histogram[x].z != 0 && histogram[x].z > yHistogramSpace)
    {
        imageStore ( textureOut , pixelCoord , vec4(0,0,1, 1));
    }
    
    if(histogramGray > 0 && histogram[x].w != 0 && histogram[x].w > yHistogramSpace)
    {
        imageStore ( textureOut , pixelCoord , vec4(1,1,1, 1));
    }
    
}