
in vec3 texCoords;

void main()
{
	gl_FragColor.rgb = (texCoords + vec3(0.5773502691896258)) * 0.8660254037844385;
	/*if (abs(texCoords.x - 0.5) < 0.01)
		gl_FragColor.rgb = vec3(1,1,1);
	else
		gl_FragColor.rgb = vec3(0,0,0);*/
}
