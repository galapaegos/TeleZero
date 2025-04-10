#version 130

uniform usampler2D image;

void main()
{
	vec3 color = texture(image, gl_TexCoord[0].st).rgb;
	color /= vec3(255.f);
	
	gl_FragColor = vec4(color, 1);
}
