//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int renderMode;

uniform float minMag;
uniform float maxMag;
uniform float angleRange;
uniform float filterAngle;

#define PI 3.14159265359

void main()
{
    float SobelKernel[9] = float[9](  -1, -2, -1, 
                                       0,  0,  0,
                                       1,  2,  1);

    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    ivec2 imgSize = imageSize(textureIn);

    float gradX = 0;
    float gradY = 0;

    int kx=0;
    for(int xx = pixelCoord.x -1; xx <= pixelCoord.x + 1; xx++)
    {

        int ky=0;
        for(int yy = pixelCoord.y -1; yy <= pixelCoord.y + 1; yy++)
        {
            int kiX = ky * 3 + kx;
            int kiY = kx * 3 + ky;
            
            float weightX = SobelKernel[kiX];
            float weightY = SobelKernel[kiY];
            
            ivec2 samplePos = ivec2(xx, yy);
            if(samplePos.x < 0) samplePos.x=0;
            if(samplePos.y < 0) samplePos.y=0;
            if(samplePos.x > imgSize.x-1) samplePos.x=imgSize.x-1;
            if(samplePos.y > imgSize.y-1) samplePos.y=imgSize.y-1;

            vec3 pixelColor = imageLoad(textureIn, samplePos).rgb;
            float grayScale = (pixelColor.r + pixelColor.g + pixelColor.b) * 0.3333;

            gradX += grayScale * weightX;         
            gradY += grayScale * weightY;
            ky++;
        }
        kx++;
    }
    float magnitude = length(vec2(gradX, gradY));
    float angle = atan(gradY, gradX);
    
    vec3 color = vec3(0);

    if(magnitude > maxMag) magnitude=0;
    if(magnitude < minMag) magnitude=0;

    float halfRange = angleRange/2;
    float minAngle = filterAngle - halfRange;
    float maxAngle = filterAngle + halfRange;
    if(angle > maxAngle) magnitude=0;
    if(angle < minAngle) magnitude=0;

    if(renderMode==0)
    {
        color = vec3(magnitude);        
    }
    else if (renderMode == 1)
    {
        color = vec3(gradX);
    }
    else if (renderMode == 2)
    {
        color = vec3(gradY);
    }
    else if (renderMode == 3)
    {
        color = vec3(gradX, gradY, 0);

    }
    else if (renderMode == 4)
    {
        color = vec3(angle / PI);
    }

    imageStore ( textureOut , pixelCoord , vec4(color, 1));
}