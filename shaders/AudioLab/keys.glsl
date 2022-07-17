//inputs
#version 440
//output
layout ( binding = 1 , rgba16f ) uniform image2D textureOut;

layout ( local_size_x = 32 , local_size_y = 32) in;

uniform sampler2D keysTexture;
uniform int mousePressed;
uniform int pressedKey;
uniform vec2 mousePosition;
uniform vec2 keysWindowSize;

const float zoomLevel = 1;


void main()
{
    ivec2 outputSize = imageSize(textureOut);

    ivec2 pixelCoord = ivec2 ( gl_GlobalInvocationID.xy );
    vec2 uv = vec2(pixelCoord) / outputSize;
    
    vec3 keysColor = texture(keysTexture, uv).rgb;
    
    vec2 uvCartesian = (uv - 0.5f) * 2.0f;

    int pixelKey = int(uv.y * 12.0f);
    
    vec2 windowPositionCartesian = uvCartesian * (keysWindowSize/2);
    if(mousePressed>0 && mousePosition.x > 0)
    {
        vec2 mousePositionCartesian = mousePosition - vec2(keysWindowSize/2);
        float dist = distance(mousePositionCartesian, windowPositionCartesian);
        if(dist<10) keysColor = vec3(1,0,0);
        
        if(pixelKey == pressedKey)
        {
            keysColor = vec3(0.4);
        }
    }

    imageStore ( textureOut , pixelCoord , vec4(keysColor, 1));
}