#version 130

uniform usampler2D image;

uniform int channels;
uniform int color_order;

void main()
{
	if(channels == 3 && color_order == 0) {
		vec3 color = texture(image, gl_TexCoord[0].st).rgb;
		color /= vec3(255.f);
	
		gl_FragColor = vec4(color, 1);
	}
	
	if(channels == 3 && color_order == 1) {
		vec3 color = texture(image, gl_TexCoord[0].st).bgr;
		color /= vec3(255.f);
	
		gl_FragColor = vec4(color, 1);
	}
	
	if(channels == 4 && color_order == 0) {
		vec4 color = texture(image, gl_TexCoord[0].st).rgba;
		color /= vec4(255.f);
	
		gl_FragColor = color;
	}
	
	if(channels == 4 && color_order == 1) {
		vec4 color = texture(image, gl_TexCoord[0].st).rgba;
		color = color.bgra;
		color /= vec4(255.f);
	
		gl_FragColor = color;
	}
	
	if(channels == 4 && color_order == 2) {
		vec4 color = texture(image, gl_TexCoord[0].st).rgba;
		color = color.abgr;
		color /= vec4(255.f);
	
		gl_FragColor = color;
	}
	
	if(channels == 4 && color_order == 3) {
		vec4 color = texture(image, gl_TexCoord[0].st).rgba;
		color = color.bgra;
		color /= vec4(255.f);
	
		gl_FragColor = color;
	}
}
