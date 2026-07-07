#shader vertex
#version 330 core

layout(location = 0) in vec2 position;

out vec2 v_LocalPosition;

// Variables given by code
uniform vec2 u_Position;
uniform vec2 u_Scale;

void main()
{
    vec2 pos = position;

    pos *= u_Scale;
    pos += u_Position;

    v_LocalPosition = position;

    gl_Position = vec4(pos, 0.0, 1.0);
}

#shader fragment
#version 330 core

out vec4 color; // Variable for the resulting color

in vec2 v_LocalPosition;

uniform vec4 desiredColor;
uniform int u_Shape;
uniform float u_CornerRadius;
uniform vec2 u_Scale;
uniform float u_Aspect;

void main()
{
    vec2 p = v_LocalPosition;
    p.x *= 1.0 / max(u_Aspect, 1e-4);

    vec2 halfSize = vec2(0.5, 0.5);

    if (u_Shape == 1)
    {
        vec2 q = abs(p) - halfSize + vec2(u_CornerRadius, u_CornerRadius);
        float dist = length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - u_CornerRadius;

        if (dist > 0.0)
            discard;
    }
    else if (u_Shape == 2)
    {
        if (length(p) > 0.5)
            discard;
    }
    else if (abs(p.x) > halfSize.x || abs(p.y) > halfSize.y)
    {
        discard;
    }

    color = vec4(desiredColor);
}