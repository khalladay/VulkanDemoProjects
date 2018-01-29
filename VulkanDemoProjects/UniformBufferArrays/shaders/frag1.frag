#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 0) uniform DATA
{
	vec4 baseColor;
	int onOff;
	vec4 colorTint;
}data;

layout(location=0) out vec4 outColor;

void main()
{
	outColor = data.baseColor * data.colorTint * (float)data.onOff;
}