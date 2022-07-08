//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;

layout ( local_size_x = 32 , local_size_y = 32) in;


layout(std430, binding=0) buffer histogramBuffer {
    ivec4 histogram[];
};

layout(std430, binding=1) buffer boundsBuffer {
    // ivec4 minBound;
    ivec4 maxBound;
};

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec3 color = imageLoad(textureIn, pixelCoord).rgb;
    int r = int(color.r * 255.0f);
    int g = int(color.g * 255.0f);
    int b = int(color.b * 255.0f);
    int grayScale = int((color.r + color.g + color.b) * 0.33333f * 255.0f);
    

    if(r>0) 
    {
        int newR  = atomicAdd(histogram[r].r, 1);
        // atomicMin(minBound.r, newR);
        atomicMax(maxBound.r, newR);
    }
    if(g>0) 
    {
        int newG  = atomicAdd(histogram[g].g, 1);
        // atomicMin(minBound.g, newG);
        atomicMax(maxBound.g, newG);
    }
    if(b>0) 
    {
        int newB  = atomicAdd(histogram[b].b, 1);
        // atomicMin(minBound.b, newB);
        atomicMax(maxBound.b, newB);
    }
    if(grayScale>0) 
    {
        int newGrayScale = atomicAdd(histogram[grayScale].a, 1);
        // atomicMin(minBound.a, newGrayScale);
        atomicMax(maxBound.a, newGrayScale);
    }

    
    
    
}