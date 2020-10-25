// Command sequencer

CONSOLE_CALL PROC_GAMEINFO( char ** argv )
{
	con_parse_start( argv );
	const char *txt = con_parse_get_buffer( argv, NULL, "path" );
	con_end( argv );

	if( txt ) fs_set_gameinfo( txt );
}

CONSOLE_CALL PROC_SETGROUP( char ** argv )
{
	con_parse_start( argv );
	const int id = con_parse_read_int( argv, 0, 0, 31, 1, "groupid" );
	const char *name = con_parse_get_buffer( argv, NULL, "visgroup name" );
	con_end( argv );
	
	tar_set_group( id, name );
}

CONSOLE_CALL PROC_LOADVMF( char ** argv )
{
	con_parse_start( argv );
	const char *path = con_parse_get_buffer( argv, NULL, "map file" );
	con_end( argv );
	
	if( !path )
	{
		fprintf( stderr, "no path received\n" );
		abort();
		
		// TODO: Use this code path to load the map from args
	}
	
	tar_setvmf( path );
	tar_bake();
}

char proc_src_vert[256] = { 0x00 };
char proc_src_frag[256] = { 0x00 };

CONSOLE_CALL PROC_VERT_SOURCE( char ** argv )
{
	con_parse_start( argv );
	const char *path = con_parse_get_buffer( argv, NULL, "path" );
	con_end( argv );
	
	strcpy( proc_src_vert, path );
}

CONSOLE_CALL PROC_FRAG_SOURCE( char ** argv )
{
	con_parse_start( argv );
	const char *path = con_parse_get_buffer( argv, NULL, "path" );
	con_end( argv );
	
	strcpy( proc_src_frag, path );
}

CONSOLE_CALL PROC_SHADER_COMPILE( char ** argv )
{
	con_parse_start( argv );
	const char *name = con_parse_get_buffer( argv, NULL, "name" );
	con_end( argv );
	
	tar_compile_shader( name, proc_src_vert, proc_src_frag );
}

CONSOLE_CALL PROC_DEPTH_ON( char ** argv )
{
	con_parse_start( argv );
	const int sw = con_parse_read_int( argv, 1, 0, 1, 1, "off/on (0/1)" );
	con_end( argv );
	
	if( sw )
	{
		glEnable( GL_DEPTH_TEST );
	}
	else
	{
		glDisable( GL_DEPTH_TEST );
	}
}

CONSOLE_CALL PROC_RENDER_GROUPS( char ** argv )
{
	con_parse_start( argv );
	// ... variadic
	con_end( argv );
	
	uint32_t bmsk = 0x00;
	
	// Loop args
	while(1)
	{
		int id = con_parse_read_int( argv, 32, 0, 32, 1, NULL );
		if( id == 32 ) break;
		
		// RENDER
		bmsk |= 0x1 << id;
	}
	
	if( !bmsk ) bmsk = ~bmsk;
	
	tar_renderfiltered( bmsk );
}

CONSOLE_CALL PROC_SETSHADER( char ** argv )
{
	con_parse_start( argv );
	const char *name = con_parse_get_buffer( argv, NULL, "name" );
	con_end( argv );
	
	if( !name )
	{
		fprintf( stderr, "shader name required\n" );
		t_quit();
	}
	
	tar_shader_t *shader = tar_get_shader( name );
	
	if( !shader )
	{
		fprintf( stderr, "shader not found (%s)\n", name );
		t_quit();
	}
	
	tar_setshader( shader->shader );
}

CONSOLE_CALL PROC_CREATEBUFFER( char ** argv )
{
	con_parse_start( argv );
	const char *type = con_parse_read_string( argv, NULL, "type" );
	const int w = con_parse_read_int( argv, 0, 0, 4096, 1, "width" );
	const int h = con_parse_read_int( argv, 0, 0, 4096, 1, "height" );
	const char *name = con_parse_get_buffer( argv, NULL, "name" );
	con_end( argv );
	
	if( !w || !h || !name )
	{
		fprintf( stderr, "Framebuffer setup missing required parameters\n" );
		t_quit();
	}
	
	EBufferType_t btype = k_EBufferType_error;
	
	if( !strcmp( type, "gbuffer" ) )
	{
		btype = k_EBufferType_gbuffer;
	} 
	else if( !strcmp( type, "rgba" ) )
	{
		btype = k_EBufferType_rgba;
	}
	
	if( btype )
	{
		tar_makebuffer( name, btype, w, h );
		printf( "Created buffer(%s), @%ix%i: '%s'\n", type, w,h, name );
	}
	else
	{
		fprintf( stderr, "Framebuffer created with invalid type\n" );
		t_quit();
	}
}

CONSOLE_CALL PROC_DRAWTARGET( char ** argv )
{
	con_parse_start( argv );
	const char *name = con_parse_get_buffer( argv, "default", "name" );
	con_end( argv );
	
	tar_drawtarget( name );
}

CONSOLE_CALL PROC_SHADER_USE( char ** argv )
{
	con_parse_start( argv );
	const char *name = con_parse_get_buffer( argv, NULL, "name" );
	con_end( argv );
	
	if( !name )
	{
		fprintf( stderr, "Shader name required to be set as active\n" );
		t_quit();
	}
	
	tar_shader_t *shader = tar_get_shader( name );
	if( !shader )
	{
		fprintf( stderr, "Shader not found\n" );
		t_quit();
	}
	
	tar_useprogram( shader );
}

CONSOLE_CALL PROC_CLEAR( char ** argv )
{
	con_parse_start( argv );
	con_end( argv );
	tar_clear();
}

CONSOLE_CALL PROC_LINKTEX( char ** argv )
{
	con_parse_start( argv );
	const char *buffer = con_parse_read_string( argv, NULL, "buffer name" );
	const int channel = con_parse_read_int( argv, 0, 0, 0, 0, "texture ID" );
	const int texid = con_parse_read_int( argv, 0, 0, 10, 1, "opengl texture id" );
	const char *uniform = con_parse_read_string( argv, NULL, "uniform to bind to" );
	con_end( argv );
	
	if( !uniform || !buffer )
	{
		fprintf( stderr, "missing required params to link texture\n" );
		t_quit();
	}
	
	tar_fb_to_sampler( buffer, channel, texid, uniform );
}

CONSOLE_CALL PROC_WRITE( char ** argv )
{
	con_parse_start( argv );
	const int w = con_parse_read_int( argv, 0, 0, 4096, 1, "output width" );
	const int h = con_parse_read_int( argv, 0, 0, 4096, 1, "output height" );
	const char *to = con_parse_get_buffer( argv, NULL, "File to write to (png)" );
	con_end( argv );
	
	if( !to || !w || !h ) return;
	
	render_to_png_flat( w, h, to );
}

CONSOLE_CALL PROC_DRAWQUAD( char ** argv )
{
	con_parse_start( argv );
	con_end( argv );
	tar_drawquad();
}

void cmdseq_reg(void)
{
	con_func_register( "gameinfo", 	PROC_GAMEINFO, 		"set gameinfo.txt path" );
	con_func_register( "setgroup", 	PROC_SETGROUP, 		"Register a visgroup name with ID" );
	con_func_register( "loadvmf", 	PROC_LOADVMF, 			"Load main vmf into tar" );
	con_func_register( "src_vert", 	PROC_VERT_SOURCE, 	"Set shader vertex source (compile)" );
	con_func_register( "src_frag", 	PROC_FRAG_SOURCE, 	"Set shader fragment source (compile)" );
	con_func_register( "compshader", PROC_SHADER_COMPILE, "Compile shader from set paths" );
	con_func_register( "depth", 		PROC_DEPTH_ON, 		"Toggle depth testing" );
	con_func_register( "render", 		PROC_RENDER_GROUPS,  "Render groups[variadic]" );
	con_func_register( "vmfshader",	PROC_SETSHADER, 		"Bind a shader to be used for VMF" );
	con_func_register( "mkbuffer",	PROC_CREATEBUFFER,	"Create a framebuffer" );
	con_func_register( "target",		PROC_DRAWTARGET,		"Bind framebuffer" );
	con_func_register( "program",		PROC_SHADER_USE,		"Use a compiled shader" );
	con_func_register( "clear",		PROC_CLEAR,				"Clears bound buffer" );
	con_func_register( "linkfbtex",	PROC_LINKTEX,			"Link framebuffer texture to uniform" );
	con_func_register( "savepng",		PROC_WRITE,			"Save bound framebuffer to flat png" );
	con_func_register( "drawquad",	PROC_DRAWQUAD,			"Draws ss quad" );
}
