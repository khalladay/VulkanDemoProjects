#version 450 core
#extension GL_ARB_separate_shader_objects : enable

struct Data48
{
	float r;
	vec4 colorB;
	int x;
};

layout(binding = 0, set = 0) uniform DATA_48
{
	Data48 data[8];
}data;

layout(push_constant) uniform PER_OBJECT 
{ 
	int dataIdx;
}pc;


layout(location=0) out vec4 outColor;

void main()
{
	float red = data.data[pc.dataIdx].r;
	float intCast = data.data[pc.dataIdx].x;
	vec4 colorA =  vec4(red, intCast, intCast, intCast);
	outColor = data.data[pc.dataIdx].colorB * colorA;
}