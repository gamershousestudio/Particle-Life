#shader vertex
#version 330 core

layout(location = 0) in vec2 position;

// Variables given by code
uniform vec2 u_Position;
uniform vec2 u_Scale;

void main()
{
    vec2 pos = position;

    pos *= u_Scale; // Scale circle
    pos += u_Position; // Move to position

    // Tells OpenGL what the position corrisponds to
    gl_Position = vec4(pos, 0.0, 1.0);
}

#shader fragment
#version 330 core

out vec4 color; // Variable for the resulting color

uniform vec4 desiredColor;

void main()
{
    // Sets rendering color
    color = vec4(desiredColor);
}