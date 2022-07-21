//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;


#define PI 3.14159265359
uniform float threshold;

void main()
{

    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    imageStore ( textureOut , pixelCoord , vec4(0,0,0, 1));
    vec2 pixelValue = imageLoad(textureIn, pixelCoord).rg;
    
    if(pixelValue.g==1) //Weak pixel
    {
        bool found=false;
        for(int y=-1; y<=1; y++)
        {
            for(int x=-1; x<=1; x++)
            {
                if(imageLoad(textureIn, pixelCoord + ivec2(x, y)).r == 1) //Strong pixel
                {
                    found=true;
                    break;
                }
            }
            if(found)break;
        }
        if(found) 
        {
            imageStore(textureOut, pixelCoord, vec4(1));
        }
    }
    else if(pixelValue.r==1)
    {
        imageStore(textureOut, pixelCoord, vec4(1));
    }

    
}