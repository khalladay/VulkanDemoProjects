#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 0) uniform DATA
{
	int onOff;
	float baseR;
	vec4 tintCol;
	float baseG;
	float baseB;
	float baseA;
	
}data;

layout(location=0) out vec4 outColor;

void main()
{
	vec4 baseCol = vec4(data.baseR, data.baseG, data.baseB, data.baseA);
	outColor = baseCol * data.tintCol * (float)data.onOff;
}