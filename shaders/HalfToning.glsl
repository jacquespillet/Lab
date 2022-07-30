//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int size;
uniform float intensity;

uniform int grayScale;

uniform mat3 rotation;
uniform mat3 rotationR;
uniform mat3 rotationG;
uniform mat3 rotationB;


layout(std430, binding=0) buffer hBuffer {
    float h[];
};
void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    ivec2 imgSize = imageSize(textureIn);

    vec2 pixelCoordCartesian = vec2(pixelCoord - (imgSize/2));
    if(grayScale>0)
    {
        //Sample Mask
        vec2 maskCoordF = (rotation * vec3(pixelCoordCartesian, 1)).xy;
        maskCoordF += vec2(imgSize/2);
        ivec2 maskCoordI = ivec2(maskCoordF) % ivec2(size);
        int maskInx = int(maskCoordI.y * size + maskCoordI.x);
        float mask = h[maskInx] * intensity;

        //Sample Color    
        vec3 color = imageLoad(textureIn, pixelCoord).rgb;
        float grayScale = (color.r + color.g + color.b) * 0.3333f;
        
        float v = grayScale + mask;

        if(v > 1)
        {
            imageStore ( textureOut , pixelCoord , vec4(1));
        }
        else
        {
            imageStore ( textureOut , pixelCoord , vec4(0,0,0,1));
        }
    }
    else
    {
        vec2 maskCoordRedF = (rotationR * vec3(pixelCoordCartesian, 1)).xy;
        maskCoordRedF += vec2(imgSize/2);
        ivec2 maskCoordRedI = ivec2(maskCoordRedF) % ivec2(size);
        int maskInxRed = int(maskCoordRedI.y * size + maskCoordRedI.x);
        float maskRed = h[maskInxRed] * intensity;        

        vec2 maskCoordGreenF = (rotationG * vec3(pixelCoordCartesian, 1)).xy;
        maskCoordGreenF += vec2(imgSize/2);
        ivec2 maskCoordGreenI = ivec2(maskCoordGreenF) % ivec2(size);
        int maskInxGreen = int(maskCoordGreenI.y * size + maskCoordGreenI.x);
        float maskGreen = h[maskInxGreen] * intensity;        

        vec2 maskCoordBlueF = (rotationB * vec3(pixelCoordCartesian, 1)).xy;
        maskCoordBlueF += vec2(imgSize/2);
        ivec2 maskCoordBlueI = ivec2(maskCoordBlueF) % ivec2(size);
        int maskInxBlue = int(maskCoordBlueI.y * size + maskCoordBlueI.x);
        float maskBlue = h[maskInxBlue] * intensity;        


        //Sample Color    
        vec3 color = imageLoad(textureIn, pixelCoord).rgb;
        
        float redV = color.r + maskRed;
        float greenV = color.g + maskGreen;
        float blueV = color.b + maskBlue;

        imageStore ( textureOut , pixelCoord , vec4(0,0,0,1));
        
        if(redV > 1)imageStore ( textureOut , pixelCoord , vec4(color.r,0,0,1));
        if(greenV > 1)imageStore ( textureOut , pixelCoord , vec4(0,color.g,0,1));
        if(blueV > 1)imageStore ( textureOut , pixelCoord , vec4(0,0,color.b,1));
    }
    
    
}