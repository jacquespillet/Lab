//inputs
#version 440
//output
layout ( binding = 0 , rgba16f ) uniform image2D textureIn;
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform int vertical;


void sort2(inout vec4 a0, inout vec4 a1) {
	vec4 b0 = min(a0, a1);
	vec4 b1 = max(a0, a1);
	a0 = b0;
	a1 = b1;
}

void sort(inout vec4 a0, inout vec4 a1, inout vec4 a2, inout vec4 a3, inout vec4 a4) {
	sort2(a0, a1);
	sort2(a3, a4);
	sort2(a0, a2);
	sort2(a1, a2);
	sort2(a0, a3);
	sort2(a2, a3);
	sort2(a1, a4);
	sort2(a1, a2);
	sort2(a3, a4);
}

void main()
{
    
    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
            
    vec4 c0 = imageLoad(textureIn, pixelCoord + ivec2(-2,0) );
    vec4 c1 = imageLoad(textureIn, pixelCoord + ivec2(-1,0) );
    vec4 c2 = imageLoad(textureIn, pixelCoord + ivec2( 0,0) );
    vec4 c3 = imageLoad(textureIn, pixelCoord + ivec2(-1,0) );
    vec4 c4 = imageLoad(textureIn, pixelCoord + ivec2(-2,0) );
    
    vec4 c5 = imageLoad(textureIn, pixelCoord + ivec2(0,-2) );
    vec4 c6 = imageLoad(textureIn, pixelCoord + ivec2(0,-1) );
    vec4 c7 = imageLoad(textureIn, pixelCoord + ivec2(0,1) );
    vec4 c8 = imageLoad(textureIn, pixelCoord + ivec2(0,2) );
    
    sort(c0, c1, c2, c3, c4);
    sort(c5, c6, c2, c7, c8);
    
    vec3 color = c2.rgb;
    imageStore ( textureOut , pixelCoord , vec4(color, 0));
}