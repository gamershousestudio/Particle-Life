#shader vertex
#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 instancePosition;
layout(location = 2) in float instanceRadius;
layout(location = 3) in vec4 instanceColor;

uniform float u_Aspect;

out vec4 v_Color;

void main()
{
    vec2 pos = position;

    pos *= instanceRadius; // Scale circle
    pos.x /= u_Aspect; // Fix oval distortion
    pos += instancePosition; // Move to position

    gl_Position = vec4(pos, 0.0, 1.0);
    v_Color = instanceColor;
}

#shader fragment
#version 330 core

in vec4 v_Color;
out vec4 color;

void main()
{
    color = v_Color;
}