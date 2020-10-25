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
	
	printf( "mask: %u\n", bmsk );
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
}
