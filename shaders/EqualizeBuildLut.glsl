//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 255 , local_size_y = 1) in;

layout(std430, binding=0) buffer histogramBuffer {
    ivec4 histogram[];
};

layout(std430, binding=2) buffer pdfBuffer {
    vec4 pdf[];
};

layout(std430, binding=3) buffer lutBuffer {
    ivec4 lut[];
};

uniform float textureSize;

void main()
{
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    if(pixelCoord.x==0 && pixelCoord.y==0)
    {
        vec4 pdf = vec4(histogram[0]) / textureSize;

        lut[0].r = int(round(255.0f * pdf.r));
        lut[0].g = int(round(255.0f * pdf.g));
        lut[0].b = int(round(255.0f * pdf.b));
        lut[0].a = int(round(255.0f * pdf.a));
        ivec4 sum=ivec4(0);
        for(int i=1; i<255; i++)
        {
            sum += histogram[i];
            lut[i].r = max(0, min(int(round(float(sum.r)  / textureSize * 255.0f) ), 255));
            lut[i].g = max(0, min(int(round(float(sum.g)  / textureSize * 255.0f) ), 255));
            lut[i].b = max(0, min(int(round(float(sum.b)  / textureSize * 255.0f) ), 255));
            lut[i].a = max(0, min(int(round(float(sum.a)  / textureSize * 255.0f) ), 255));
            
            // pdf = vec4(histogram[i]) / textureSize;
            // lut[i].r = int(round(255.0f * pdf.r)) + lut[i-1].r;
            // lut[i].g = int(round(255.0f * pdf.g)) + lut[i-1].g;
            // lut[i].b = int(round(255.0f * pdf.b)) + lut[i-1].b;
            // lut[i].a = int(round(255.0f * pdf.a)) + lut[i-1].a;
        }
    }
}