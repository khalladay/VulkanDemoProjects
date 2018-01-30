#version 450 core
#extension GL_ARB_separate_shader_objects : enable

struct Data64
{
	vec4 colorA;
	vec4 colorB;
};

layout(binding = 0, set = 0) uniform DATA_64
{
	Data64 testing[8];
}data;

layout(push_constant) uniform PER_OBJECT 
{ 
	int dataIdx;
}pc;


layout(location=0) out vec4 outColor;

void main()
{
	outColor = data.testing[pc.dataIdx].colorA * data.testing[pc.dataIdx].colorB;
}