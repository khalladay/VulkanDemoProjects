#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler samp;
layout(set = 0, binding = 1) uniform texture2D textures[78];

layout(push_constant) uniform PER_OBJECT 
{ 
	int imgIdx;
}pc;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 fragUV;

void main()
{
	outColor = texture(sampler2D(textures[pc.imgIdx], samp), fragUV);
}