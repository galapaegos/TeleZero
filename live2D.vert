void main()
{
	gl_TexCoord[0] = gl_TextureMatrix[0]*gl_MultiTexCoord0;
	gl_TexCoord[0].y = 1.0 - gl_TexCoord[0].y;
	gl_Position = ftransform();
}
