#version 450 core
#extension GL_ARB_separate_shader_objects : enable

struct Data64
{
	float r;
	vec4 colorB;
	float g;
	float b;
	int a;	
};

layout(binding = 0, set = 0) uniform DATA_64
{
	Data64 data[8];
}data;

layout(push_constant) uniform PER_OBJECT 
{ 
	int dataIdx;
}pc;


layout(location=0) out vec4 outColor;

void main()
{
	vec4 colorA =  vec4(data.data[pc.dataIdx].r, data.data[pc.dataIdx].g, data.data[pc.dataIdx].b, data.data[pc.dataIdx].a);
	outColor = data.data[pc.dataIdx].colorB * colorA;
}