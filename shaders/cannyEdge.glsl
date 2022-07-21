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

    vec3 currentColor = imageLoad(textureIn, pixelCoord).rgb;
    
    float angle = currentColor.g * (180.0 / PI);
    if(angle <0) angle += 180;

    float magnitude = currentColor.r;

    int i = pixelCoord.x;
    int j = pixelCoord.y;

    float q = 1;
    float r = 1;
    if ((0 <= angle && angle < 22.5) || (157.5 <= angle && angle <= 180))
    {
        q = imageLoad(textureIn, ivec2(i, j+1)).r;
        r = imageLoad(textureIn, ivec2(i, j-1)).r;
    }
    else if (22.5 <= angle && angle < 67.5)
    {
        q = imageLoad(textureIn, ivec2(i+1, j-1)).r;
        r = imageLoad(textureIn, ivec2(i-1, j+1)).r;
    }
    else if (67.5 <= angle && angle < 112.5)
    {
        q = imageLoad(textureIn, ivec2(i+1, j)).r;
        r = imageLoad(textureIn, ivec2(i-1, j)).r;
    }
    else if (112.5 <= angle && angle < 157.5)
    {
        q = imageLoad(textureIn, ivec2(i-1, j-1)).r;
        r = imageLoad(textureIn, ivec2(i+1, j+1)).r;
    }

    if ((magnitude >= q) && (magnitude >= r))
        imageStore ( textureOut , pixelCoord , vec4(magnitude.xxx, 1));
    // if(magnitude > threshold)
    {
    //     float edgeCategoryFloat = float(edgeCategory) / 4.0f;
    }
}