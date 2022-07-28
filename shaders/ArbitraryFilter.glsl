//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int sizeX;
uniform int sizeY;


layout(std430, binding=0) buffer kernelBuffer {
    float kernel[];
};
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    ivec2 imgSize = imageSize(textureIn);

    int halfSizeX = int(floor( float(sizeX) /2.0f));
    int halfSizeY = int(floor( float(sizeY) /2.0f));
    
    vec3 color = vec3(0,0,0);
    int x=0;
    for(int xx = pixelCoord.x - halfSizeX; xx <= pixelCoord.x + halfSizeX; xx++)
    {
        x++;
        int y=0;
        for(int yy = pixelCoord.y - halfSizeY; yy <= pixelCoord.y + halfSizeY; yy++)
        {
            int flatInx = y * sizeX + x;

            ivec2 samplePos = ivec2(xx, yy);
            if(samplePos.x < 0) samplePos.x=0;
            if(samplePos.y < 0) samplePos.y=0;
            if(samplePos.x > imgSize.x-1) samplePos.x=imgSize.x-1;
            if(samplePos.y > imgSize.y-1) samplePos.y=imgSize.y-1;

            float weight = kernel[flatInx];
            color += imageLoad(textureIn, samplePos).rgb * weight;
            y++;
        }
    }
    imageStore ( textureOut , pixelCoord , vec4(color, 1));
}