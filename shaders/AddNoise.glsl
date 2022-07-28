//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform float density;
uniform float intensity;
uniform int randomColor;

uint wang_hash(inout uint seed)
{
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float RandomUnilateral(inout uint State)
{
    return float(wang_hash(State)) / 4294967296.0;
}

float RandomBilateral(inout uint State)
{
    return RandomUnilateral(State) * 2.0f - 1.0f;
}

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    uint randomState = (pixelCoord.x * 1973 + pixelCoord.y * 9277) | 1; 

    vec3 color = imageLoad(textureIn, pixelCoord).rgb;
    float random = RandomUnilateral(randomState);

    vec3 noiseColor = vec3(RandomBilateral(randomState));
    if(randomColor>0)
    {
        noiseColor = vec3(RandomBilateral(randomState), RandomBilateral(randomState), RandomBilateral(randomState));
    }
    
    if(random < density) color += noiseColor * intensity;

    imageStore ( textureOut , pixelCoord , vec4(color, 1));
}