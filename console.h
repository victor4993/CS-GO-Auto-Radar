#undef __min
#undef __max
#define __min(X,Y) ((X) < (Y) ? (X) : (Y))
#define __max(X,Y) ((X) > (Y) ? (X) : (Y))

/*
								VIRTUAL CONSOLE INTERFACE
	===================================================================================
	
*/

#define CONSOLE_CALL void

void execute_console_input(const char* strInput);

// Stored function definition for the library
typedef struct
{
	char strName[128];
	
	void (*ptrFunc)( char ** );
	
	void (*ptrCVARFUNC)( char **, void * );
	void  *ptrCVARDATA;
	
	// Built by parsers when registering
	int	iCountParameters;
	char *strDescription;
	char **strParamInfo;
	char *strParamFormatString;
} con_func_definition;

// Global that points to the currently being registered definition
con_func_definition* ptr_current_registering;

// 'Table' of function definitions.
// TODO(LOW): Improve storage method and searching algorthims
con_func_definition temp_conlib[128] = { 0 };
con_func_definition* alloc_def( const char* strName )
{
	for( int i = 0; i < sizeof temp_conlib; i++ )
	{
		if( !temp_conlib[i].ptrFunc && !temp_conlib[i].ptrCVARFUNC )
			return &temp_conlib[i];
	}
	return NULL;
}
con_func_definition* find_def( const char *strName )
{
	for( int i = 0; i < 128; i++ )
	{
		if( temp_conlib[i].strName )
		{
			if( !strcmp(temp_conlib[i].strName, strName) )
			{
				return &temp_conlib[i];
			}
		}
	}
	return NULL;
}

// Initialize parsing
void con_parse_start( char **argv )
{
	if( !argv )
	{
		// Registration mode
	}
}

int con_parse_end( char **argv )
{
	if( !argv ) return 0;
	return 1;
}

void write_param_definition( const char *strDefinition )
{
	ptr_current_registering->iCountParameters++;
	// Add definition to string	
}

// Advance cursor to next token and return token
char *con_prep_tok( char **stream )
{
	if( !*stream || !**stream ) return NULL; // EOF
	
	char* data = *stream;
	
	int token = 1;
	while( **stream )
	{
		// Fill whitespace with null
		if( isspace( **stream) )
		{
			token = 0;
			**stream = 0x00;
		} 
		else 
		{
			if( !token )
			{
				break;
			}
		}
		(*stream)++;
	}
	
	return data;
}

void con_paramdef_append( const char* fmt, ... )
{
	char* buffer = malloc(512);
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer,512, fmt, args);
	va_end(args);

	sb_push(ptr_current_registering->strParamInfo, buffer);
}

int con_parse_read_int( char** stream, int iDefault, int iMin, int iMax, int clamp, const char* strInfo )
{
	if( !stream )
	{
		// Registration mode
		if( clamp ) con_paramdef_append( "int32( %s ): defualt'%i' range[%i->%i]", strInfo, iDefault, iMin, iMax );
		else 			con_paramdef_append( "int32( %s ): default'%i'", strInfo, iDefault );
		
		return 0;
	}
	char* data = con_prep_tok( stream );
	if( !data ) 
	{ 
		return iDefault; 
	}
	return clamp? __min(iMax, __max(iMin, atoi(data))): atoi(data);
}

uint64_t con_parse_read_u64( char ** stream, uint64_t unDefault, const char *strInfo )
{
	if( !stream )
	{
		con_paramdef_append("uint64( %s ): default'%lu'", strInfo, unDefault);
		return 0;
	}
	char *data = con_prep_tok( stream );
	if( !data ) 
	{ 
		return unDefault; 
	}
	
	//todo: check if this can support uint64_t sizes
	return strtoul(data, NULL, 0);
}

// STD console read float
float con_parse_read_float( char** stream, float fDefault, float fMin, float fMax, int clamp, const char* strInfo )
{
	if( !stream )
	{
		if( clamp ) con_paramdef_append("float32( %s ): defualt'%.3f' range[%.3f->%.3f]", strInfo, fDefault, fMin, fMax);
		else 			con_paramdef_append("float32( %s ): default'%.3f'", strInfo, fDefault);
		return 0;
	}
	char* data = con_prep_tok(stream);
	if( !data ) 
	{ 
		return fDefault; 
	}
	return clamp? __min(fMax, __max(fMin, atof(data))): atof(data);
}

// Read string from input buffer
const char* con_parse_read_string( char **stream, const char *strDefault, const char *strInfo )
{
	if( !stream )
	{
		// Registration mode
		con_paramdef_append("string( %s ): defualt'%s'", strInfo, strDefault);
		return 0;
	}
	char* data = con_prep_tok(stream);
	if(!data) 
	{ 
		return strDefault; 
	}
	return data;
}

const char *con_parse_get_buffer( char **stream, const char *strDefault, const char *strInfo )
{
	if( !stream )
	{
		con_paramdef_append("buffer( %s ): default'%s'", strInfo, strDefault);
		return 0;
	}
	char *data = *stream;
	if(!data) 
	{ 
		return strDefault; 
	}
	
	return data;
}

// Register a function by calling it with a NULL pointer
void con_func_register( const char *strName, void(*ptrFunc)( char ** ), const char *strDescription )
{
	con_func_definition *definition = alloc_def(strName);
	ptr_current_registering = definition;

	// Copy info 
	strcpy( definition->strName, strName ); 	// Name
	
	definition->strDescription = malloc(strlen(strDescription)+1); // Reallocate description
	strcpy(definition->strDescription, strDescription); 				// Copy description
	
	definition->ptrFunc = ptrFunc;	// Function pointer

	// Perform registration call
	ptrFunc( NULL );
}

void con_cvar_std_int( char **stream, void *data )
{
	char *pch = con_prep_tok( stream );
	if( pch )
	{
		*((int*)data) = atoi( pch );
	}
}

void con_ivar_register( const char *strName, int32_t *ptrData, const char *strDescription )
{
	con_func_definition* definition = alloc_def( strName );
	ptr_current_registering = definition;
	con_paramdef_append( "std_cvar_int32: %i", 0 );
	
	strcpy( definition->strName, strName );
	definition->strDescription = malloc( strlen(strDescription)+1 );
	strcpy( definition->strDescription, strDescription );
	
	definition->ptrCVARFUNC = &con_cvar_std_int;
	definition->ptrCVARDATA = ( void * )ptrData;
	
	// No reg call
}

void con_cvar_std_float( char **stream, void *data )
{
	char *pch = con_prep_tok(stream);
	if( pch )
	{
		*((float*)data) = atof(pch);
	}
}

void con_fvar_register( const char* strName, void* ptrData, const char* strDescription )
{
	con_func_definition *definition = alloc_def(strName);
	ptr_current_registering = definition;
	con_paramdef_append("std_cvar_float32: %i", 0);
	
	strcpy(definition->strName, strName);
	definition->strDescription = malloc(strlen(strDescription)+1);
	strcpy(definition->strDescription, strDescription);
	
	definition->ptrCVARFUNC = &con_cvar_std_float;
	definition->ptrCVARDATA = ptrData;
}

#define con_end(argv) if(!con_parse_end(argv)) return;

/*

	STANDARD CONSOLE CALLS

*/

void con_run_cfg( const char *file_name )
{
	FILE* file = fopen(file_name, "r");
		
	if( file )
	{
		char line[256];

		while (fgets(line, sizeof(line), file)) {
			line[strcspn(line, "\r\n#")] = 0x00;
			
			if( line[0] != 0x00 )
			{
				execute_console_input( line );
			}
		}

		fclose(file);
	}
	else
	{
		fprintf( stderr, "'%s' not found\n", file_name );
	}
}

CONSOLE_CALL con_std_exec_cfg( char **argv )
{
	con_parse_start(argv);
	const char *cfg = con_parse_read_string( argv, NULL, "config" );
	con_end(argv);
	
	if( cfg && strlen(cfg) < 100 )
	{
		char file_name[128];
		strcpy( file_name, cfg );
		strcat( file_name, ".rat" );
		
		con_run_cfg( file_name );
	}
	else
	{
		fprintf( stderr, "Usage: exec <name>.rat\n" );
	}
}

void execute_console_input( const char *strInput )
{
	char *strBuf = malloc(strlen(strInput)+1);
	strcpy(strBuf, strInput);
	
	char *pChrHead = &strBuf[0];
	
	// Position read head
	while(*pChrHead && isspace(*(pChrHead)))
	{
		*pChrHead = 0x00; // Fill with null
		pChrHead++;
	}
	
	const char *func_name = con_parse_read_string(&pChrHead, NULL, "");
	if(!func_name) return; // empty input
	
	con_func_definition* funcDef = find_def(func_name);
	
	if( !funcDef )
	{
		fprintf( stderr, "Unkown command: %s ( Use 'list' to find commands )\n", func_name );
		return;
	}
	
	// Run function or execute cvar
	if( funcDef->ptrFunc ) ((void(*)(char**))funcDef->ptrFunc)(&pChrHead);
	else if( funcDef->ptrCVARFUNC ) ((void(*)(char**, void*))funcDef->ptrCVARFUNC)(&pChrHead, funcDef->ptrCVARDATA);
}

CONSOLE_CALL con_std_doc( char **argv )
{
	con_parse_start( argv );
	con_end( argv );
	
	con_func_definition *pfunc = temp_conlib;
	
	while( (pfunc->ptrFunc || pfunc->ptrCVARFUNC) )
	{
		printf( "\n#%s \"%s\"\n", pfunc->strName, pfunc->strDescription );
		for( int i = 0; i < sb_count( pfunc->strParamInfo ); i ++ )
		{
			printf( "%i: %s\n", i, pfunc->strParamInfo[i] );
		}

		pfunc++;
	}
}

void con_setup(void)
{
	con_func_register( "exec", con_std_exec_cfg, "execute script file (.rat)" );
	con_func_register( "docall", con_std_doc, "Write out some documentation for every registered command" );
}
