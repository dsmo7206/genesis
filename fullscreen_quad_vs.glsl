
out vec2 v2_texCoord;

const vec2 data[4] = vec2[]
(
	vec2(-1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
);

void main()
{
	v2_texCoord = data[gl_VertexID]*0.5 + vec2(0.5);
	gl_Position = vec4(data[gl_VertexID], 0.0, 1.0);
}
