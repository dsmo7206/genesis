#version 330
 
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 MV;

in vec3 vs_out_vertexPositionWorldSpace[];
in vec3 vs_out_eyeDirectionCameraSpace[];
in vec3 vs_out_lightDirectionCameraSpace[];
in vec3 vs_out_colour[];

out vec3 gs_out_normalSphereSpace;
out vec3 gs_out_colour;
out vec3 gs_out_vertexPositionWorldSpace;
out vec3 gs_out_normalCameraSpace;
out vec3 gs_out_eyeDirectionCameraSpace;
out vec3 gs_out_lightDirectionCameraSpace;
 
void main(void)
{
    const vec3 _normalSphereSpace = normalize(
		cross(
			vs_out_vertexPositionWorldSpace[1].xyz - vs_out_vertexPositionWorldSpace[0].xyz,
			vs_out_vertexPositionWorldSpace[2].xyz - vs_out_vertexPositionWorldSpace[0].xyz
		)
	);

	// Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.
	const vec3 _normalCameraSpace = (MV * vec4(_normalSphereSpace, 0)).xyz; 

	gl_Position = gl_in[0].gl_Position;
	gs_out_normalSphereSpace = _normalSphereSpace;
	gs_out_colour = vs_out_colour[0];
	gs_out_vertexPositionWorldSpace = vs_out_vertexPositionWorldSpace[0];
	gs_out_normalCameraSpace = _normalCameraSpace;
	gs_out_eyeDirectionCameraSpace = vs_out_eyeDirectionCameraSpace[0];
	gs_out_lightDirectionCameraSpace = vs_out_lightDirectionCameraSpace[0];
	EmitVertex();
 
	gl_Position = gl_in[1].gl_Position;
	gs_out_normalSphereSpace = _normalSphereSpace;
	gs_out_colour = vs_out_colour[1];
	gs_out_vertexPositionWorldSpace = vs_out_vertexPositionWorldSpace[1];
	gs_out_normalCameraSpace = _normalCameraSpace;
	gs_out_eyeDirectionCameraSpace = vs_out_eyeDirectionCameraSpace[1];
	gs_out_lightDirectionCameraSpace = vs_out_lightDirectionCameraSpace[1];
	EmitVertex();

	gl_Position = gl_in[2].gl_Position;
	gs_out_normalSphereSpace = _normalSphereSpace;
	gs_out_colour = vs_out_colour[2];
	gs_out_vertexPositionWorldSpace = vs_out_vertexPositionWorldSpace[2];
	gs_out_normalCameraSpace = _normalCameraSpace;
	gs_out_eyeDirectionCameraSpace = vs_out_eyeDirectionCameraSpace[2];
	gs_out_lightDirectionCameraSpace = vs_out_lightDirectionCameraSpace[2];
	EmitVertex();
 
    EndPrimitive();
}
