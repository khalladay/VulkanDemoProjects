#version 450 core
#extension GL_ARB_separate_shader_objects : enable

struct Data48
{
	vec4 colorA;
	vec4 colorB;
	vec4 colorC;
};

layout(binding = 0, set = 0) uniform DATA_48
{
	Data48 testing[8];
}data;

layout(push_constant) uniform PER_OBJECT 
{ 
	int dataIdx;
}pc;


layout(location=0) out vec4 outColor;

void main()
{
	outColor = data.testing[pc.dataIdx].colorA 
			+ data.testing[pc.dataIdx].colorB 
			+ data.testing[pc.dataIdx].colorC;
}