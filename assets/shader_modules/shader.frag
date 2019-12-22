#version 450

precision mediump float;
precision mediump sampler2DArray;

layout(location = 0) in vec3 UV;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2DArray ImageArray;

void main()
{			
  fragColor = texture(ImageArray, UV);
}