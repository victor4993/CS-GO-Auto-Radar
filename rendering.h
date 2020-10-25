// General purpose gbuffer
typedef struct
{
	GLuint frameBuffer;

	GLuint texPosition;	// RGB 16F
	GLuint texNormal;		// RGB 16F
	GLuint texOrigin;		// RG  16F
	
	GLuint renderBuffer;
	
	uint32_t w,h;
} gbuffer_t;

int gbuffer_setup( 
 uint32_t const w, uint32_t const h,
 GLuint *textures,
 GLuint *fb,
 GLuint *rb
) {

	glGenFramebuffers( 1, fb );
	glBindFramebuffer( GL_FRAMEBUFFER, *fb );
	
	// Position
	glGenTextures( 1, textures );
	glBindTexture( GL_TEXTURE_2D, textures[0] );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0 );
	
	// Normal
	glGenTextures( 1, textures + 1 );
	glBindTexture( GL_TEXTURE_2D, textures[1] );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textures[1], 0 );
	
	// XY origin
	glGenTextures( 1, textures + 2 );
	glBindTexture( GL_TEXTURE_2D, textures[2] );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, textures[2], 0 );
	
	unsigned int attachments[3] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2
	};
	
	glDrawBuffers( 3, attachments );
	
	glGenRenderbuffers( 1, rb );
	glBindRenderbuffer( GL_RENDERBUFFER, *rb );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h );
	glBindRenderbuffer( GL_RENDERBUFFER, 0 );

	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *rb );

	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		fprintf( stderr, "gbuffer::init: Not complete\n" );
		return 0;
	}
	
	return 1;
}

int gbuffer_init( gbuffer_t *self, uint32_t const w, uint32_t const h )
{
	self->w = w; self->h = h;
	
	// Init framebuffer
	glGenFramebuffers( 1, &self->frameBuffer );
	glBindFramebuffer( GL_FRAMEBUFFER, self->frameBuffer );
	
	// Position
	glGenTextures( 1, &self->texPosition );
	glBindTexture( GL_TEXTURE_2D, self->texPosition );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, self->w, self->h, 0, GL_RGB, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->texPosition, 0 );
	
	// Normal
	glGenTextures( 1, &self->texNormal );
	glBindTexture( GL_TEXTURE_2D, self->texNormal );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F, self->w, self->h, 0, GL_RGB, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, self->texNormal, 0 );
	
	// XY origin
	glGenTextures( 1, &self->texOrigin );
	glBindTexture( GL_TEXTURE_2D, self->texOrigin );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RG16F, self->w, self->h, 0, GL_RG, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, self->texOrigin, 0 );
	
	unsigned int attachments[3] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2
	};
	
	glDrawBuffers( 3, attachments );
	
	glGenRenderbuffers( 1, &self->renderBuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, self->renderBuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, self->w, self->h );
	glBindRenderbuffer( GL_RENDERBUFFER, 0 );

	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->renderBuffer );

	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
	{
		fprintf( stderr, "gbuffer::init: Not complete\n" );
		return 0;
	}
	
	return 1;
}

void gbuffer_use( gbuffer_t *self )
{
	glBindFramebuffer( GL_FRAMEBUFFER, self->frameBuffer );
	glViewport( 0, 0, self->w, self->h );
}
