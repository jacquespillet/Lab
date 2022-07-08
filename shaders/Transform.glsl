//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform mat3 transformMatrix;

uniform int interpolation;
uniform int flipX;
uniform int flipY;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    ivec2 imageSize = imageSize(textureIn);
    vec2 halfImageSize = vec2(imageSize) * 0.5f;
    
    ivec2 outPixelcoord = pixelCoord;
    if(flipX>0) outPixelcoord.x = imageSize.x - pixelCoord.x;
    if(flipY>0) outPixelcoord.y = imageSize.y - pixelCoord.y;

    vec2 cartesianCoords = vec2(pixelCoord) - halfImageSize;
    vec3 transformedCoord = transformMatrix * vec3(cartesianCoords, 1);
    transformedCoord/=transformedCoord.z;
    transformedCoord.xy += halfImageSize.xy;


    if(interpolation==0)
    {
        vec3 color = imageLoad(textureIn, ivec2(transformedCoord.xy)).rgb;
        imageStore ( textureOut , outPixelcoord , vec4(color, 0));
    }
    else
    {
        int x0 = int(floor(transformedCoord.x));
        int x1 = int(ceil(transformedCoord.x));
        int y0 = int(floor(transformedCoord.y));
        int y1 = int(ceil(transformedCoord.y));
        
        float alpha = fract(transformedCoord.x);
        float beta  = fract(transformedCoord.y);

        vec3 p00 = imageLoad(textureIn, ivec2(x0, y0)).rgb;
        vec3 p10 = imageLoad(textureIn, ivec2(x1, y0)).rgb;
        vec3 p01 = imageLoad(textureIn, ivec2(x0, y1)).rgb;
        vec3 p11 = imageLoad(textureIn, ivec2(x1, y1)).rgb;
        
        vec3 color = ((1.0f - beta) * p00 + beta * p01) * (1 - alpha) + 
                     ((1.0f - beta) * p10 + beta * p11) * (alpha); 
        imageStore ( textureOut , outPixelcoord , vec4(color, 0));
    }
}