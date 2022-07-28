//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int vertical;


void main()
{
    float SobelKernel[9] = float[9](  -1, -2, -1, 
                                       0,  0,  0,
                                       1,  2,  1);

    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    ivec2 imgSize = imageSize(textureIn);

    vec3 edgeColor = vec3(0,0,0);

    int kx=0;
    for(int xx = pixelCoord.x -1; xx <= pixelCoord.x + 1; xx++)
    {

        int ky=0;
        for(int yy = pixelCoord.y -1; yy <= pixelCoord.y + 1; yy++)
        {
            int ki = ky * 3 + kx;
            if(vertical==1) ki = kx * 3 + ky;

            float weight = SobelKernel[ki];
               
            ivec2 samplePos = ivec2(xx, yy);
            if(samplePos.x < 0) samplePos.x=0;
            if(samplePos.y < 0) samplePos.y=0;
            if(samplePos.x > imgSize.x-1) samplePos.x=imgSize.x-1;
            if(samplePos.y > imgSize.y-1) samplePos.y=imgSize.y-1;

            edgeColor += imageLoad(textureIn, samplePos).rgb * weight;
            
            ky++;
        }
        kx++;
    }
    vec3 color = edgeColor;
            
    imageStore ( textureOut , pixelCoord , vec4(color, 1));
}