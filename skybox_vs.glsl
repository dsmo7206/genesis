
layout(location = 0) in vec3 v3_vertPos_MS;

uniform mat4 m4_MVP;
out vec3 texCoords;

void main()
{
	texCoords = v3_vertPos_MS;
	gl_Position = m4_MVP * vec4(v3_vertPos_MS, 1.0);
}
