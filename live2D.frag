#version 130

uniform usampler2D image;

void main()
{
	float image_value = texture(image, gl_TexCoord[0].st).r;
	vec3 primary_color = vec3 (image_value, image_value, image_value);
	
	gl_FragColor = vec4(color, 1);
}
