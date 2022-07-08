//inputs
#version 440

layout ( local_size_x = 255 , local_size_y = 1) in;

layout(std430, binding=0) buffer histogramBuffer {
    ivec4 histogram[];
};

layout(std430, binding=1) buffer boundsBuffer {
    // ivec4 minBound;
    ivec4 maxBound;
};

void main()
{
    // minBound = ivec4(999999999);
    maxBound = ivec4(0);

    histogram[gl_GlobalInvocationID.x] = ivec4(0,0,0,0);
}