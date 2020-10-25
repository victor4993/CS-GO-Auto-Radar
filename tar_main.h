typedef struct {
	
	uint32_t start;
	uint32_t num;
	
} tar_drawcmd_t;

typedef struct tar_vmf_t tar_vmf_t;
typedef struct tar_model_t tar_model_t;
typedef struct tar_shader_t tar_shader_t;
typedef struct tar_buffer_t tar_buffer_t;

typedef enum
{
	k_EBufferType_error,
	k_EBufferType_gbuffer,
	k_EBufferType_rgba
} EBufferType_t;

struct tar_buffer_t
{
	char *name;
	
	EBufferType_t type;
	
	GLuint fb, rb;
	GLuint textures[3];
	
	uint32_t w,h;
};

// Render model / render instances
typedef struct 
{
	uint32_t modelid;
	mat4 		transform;
} RModel_t;

typedef struct {
	tar_vmf_t *pVmf;
	mat4 		transform;
} RInstance_t;

typedef struct {

	int vgroupid;
	
	uint32_t brush_start;
	uint32_t brush_num;

	RModel_t	*models;
	uint32_t	model_num;
	uint32_t model_cap;	//runtime

} tar_drawgroup_t;

struct tar_vmf_t {

	char 				*name;
	vdf_node_t		*source;
	
	tar_drawgroup_t groups[32];
	
	RInstance_t 		*sub_vmfs;
	uint32_t				sub_vmf_num;
	
	mat4				mCurrent;
	
	struct {
		GLuint vao_brushes;
		GLuint vbo_brushes;
		GLuint ebo_brushes;
	} gl;
};

struct tar_model_t 
{
	unsigned long szName_hash;
	char *szName;
	
	GLuint vao, vbo, ebo;
	uint32_t idx_count;
};

struct tar_shader_t
{
	char *szName;
	GLuint shader;
};

// Foreach vmf there is:
//  Brushes
//  Models
//  Subvmfs

char 		  *tar_groups[32] = {NULL};
tar_vmf_t 	tar_maps[32];
uint32_t		num_maps = 0;

tar_model_t *tar_models = NULL;
tar_shader_t *tar_shaders = NULL;
tar_buffer_t *tar_buffers = NULL;

void tar_makebuffer( const char *name, EBufferType_t const type, uint32_t const w, uint32_t const h )
{
	tar_buffer_t buffer = {
		.name = malloc( strlen( name ) + 1 ),
		.type = type
	};
	
	strcpy( buffer.name, name );

	switch( type )
	{
		case k_EBufferType_gbuffer:
		gbuffer_setup( w, h, buffer.textures, &buffer.fb, &buffer.rb );
		break;
		
		default:
		case k_EBufferType_rgba:
		break;
	}
	
	sb_push( tar_buffers, buffer );
}

void fbuffer_reset(void);
void tar_drawtarget( const char *name )
{
	if( !strcmp( name, "default" ) )
	{
		fbuffer_reset();
		return;
	}

	for( int i = 0; i < sb_count( tar_buffers ); i ++ )
	{
		if( !strcmp( name, tar_buffers[i].name ) )
		{
			tar_buffer_t *buffer = tar_buffers + i;
			glBindFramebuffer( GL_FRAMEBUFFER, buffer->fb );
			glViewport( 0, 0, buffer->w, buffer->h );
			return;
		}
	}
	
	fprintf( stderr, "framebuffer '%s' not found.. unexpected results will occur\n", name );
}

void tar_compile_shader( const char *name, const char *vert, const char *frag )
{
	tar_shader_t shader = {
		.szName = malloc( strlen( name ) + 1 ),
		.shader = shader_compile( vert, frag )
	};
	
	strcpy( shader.szName, name );
	
	sb_push( tar_shaders, shader );
}

tar_shader_t *tar_get_shader( const char *szName )
{
	for( int i = 0; i < sb_count( tar_shaders ); i ++ )
	{
		if( !strcmp( szName, tar_shaders[ i ].szName ) )
		{
			return tar_shaders + i;
		}
	}
	
	return NULL;
}

tar_shader_t *active = NULL;
void tar_useprogram( tar_shader_t *shader )
{
	glUseProgram( shader->shader );
	active = shader;
}

void tar_fb_to_sampler( const char *buffer, const uint32_t n, const uint32_t tn, const char *uniform )
{
	if( n >= 3 )
	{
		fprintf( stderr, "texture out of range\n" );
		return;
	}

	for( int i = 0; i < sb_count( tar_buffers ); i ++ )
	{
		if( !strcmp( tar_buffers[i].name, buffer ) )
		{
			tar_buffer_t *buffer = tar_buffers + i;
			
			glBindTexture( GL_TEXTURE_2D, buffer->textures[n] );
			glActiveTexture( GL_TEXTURE0+tn );
			glUniform1i( glGetUniformLocation( active->shader, uniform ), tn );
		}
	}
}

void tar_clear(void)
{
	glClearColor( 0.f, 0.f, 0.f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

// Get model pointer from cache, 0 if not present ( you should set an error model )
uint32_t _tar_getmodel( const char *szName )
{
	unsigned long hash = djb2( (unsigned char *)szName );

	for( uint32_t i = 0; i < sb_count( tar_models ); i ++ )
	{
		if( tar_models[ i ].szName_hash == hash )
		{
			if( !strcmp( tar_models[ i ].szName, szName ) )
			{
				return i;
			}
		}
	}
	
	return 0;
}

// Internal model cache
uint32_t _tar_loadmodel( const char *szName )
{
	// Make sure if error model is present
	// TODO: update to something sensible instead of segfault
	if( !sb_count( tar_models ) )
	{
		tar_model_t errmdl = {};
		sb_push( tar_models, errmdl );
	}

	// Check cache if we have it already
	uint32_t modelid = _tar_getmodel( szName );
	if( modelid ) return modelid;
	
	// We dont have it needs load
	mdl_mesh_t ctx;
	
	if( !mdl_read_fs( szName, &ctx ) )
	{
		// Set 'uv's all to 0,0 cause yeah
		for( int i = 0; i < ctx.unVertices; i ++ )
		{
			ctx.vertices[ i * 8 + 6 ] = 0.f;
			ctx.vertices[ i * 8 + 7 ] = 0.f;
		}	
		
		// Set up model header
		tar_model_t mdl;
		mdl.idx_count = ctx.unIndices;
		mdl.szName = malloc( strlen( szName ) + 1 );
		strcpy( mdl.szName, szName );
		mdl.szName_hash = djb2( ( unsigned char *) szName );
		
		printf( "cachemdl: %lu.%s\n", mdl.szName_hash, mdl.szName );
		
		// Plonk opengl data  in
		glGenVertexArrays( 1, &mdl.vao );
		glGenBuffers( 1, &mdl.vbo );
		glGenBuffers( 1, &mdl.ebo );
		glBindVertexArray( mdl.vao );
		
		glBindBuffer( GL_ARRAY_BUFFER, mdl.vbo );
		glBufferData( GL_ARRAY_BUFFER, ctx.unVertices * sizeof( solidgen_vert_t ), ctx.vertices, GL_STATIC_DRAW );
			
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mdl.ebo );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, ctx.unIndices * sizeof( uint16_t ), ctx.indices, GL_STATIC_DRAW );
		
		// co
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( solidgen_vert_t ), (void *)0 );
		glEnableVertexAttribArray( 0 );
		
		// normal
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( solidgen_vert_t ), (void *)( sizeof(float) * 3 ) );
		glEnableVertexAttribArray( 1 );
		
		// origin ( we slap these to 0 anyway, since its gonna take the transform override in the shader )
		glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( solidgen_vert_t ), (void *)( sizeof(float) * 6 ) );
		glEnableVertexAttribArray( 2 );
		
		// Free model resources ya
		mdl_free( &ctx );
		
		modelid = sb_count( tar_models );
		sb_push( tar_models, mdl );
		
		return modelid;
	}
	
	// model load failure
	fprintf( stderr, "model load failed: %s\n", szName );
	return NULL;
}

// Internal vmf load
int _tar_loadvmf( const char *szPath )
{
	if( !szPath ) return 0;

	tar_vmf_t *vmap = tar_maps + (num_maps ++);
	
	// Source
	vmap->source = vdf_open_file( szPath );
	if( !vmap->source )
	{
		fprintf( stderr, "   @tar::loadvmf::vdf_open_file()\n" );
		t_quit();
	}
	
	// Name
	vmap->name = malloc( strlen( szPath ) + 1 );
	strcpy( vmap->name, szPath );
	
	// .. Contexts allocated at tar_run
	vmap->sub_vmf_num = 0;
	uint32_t	sub_vmf_cap = 0;

	// Set transform to nothing in-case its root
	glm_mat4_identity( vmap->mCurrent );
	
	// Load sub-vmfs
	VDF_ITER( vmap->source, "entity",
		if( !strcmp( vdf_kv_get( NODE, "classname", "" ), "func_instance" ) )
		{
			// TODO: Append basepath of VMF to this file, since is relative
			const char *subname = vdf_kv_get( NODE, "file", NULL );
			if( subname )
			{
				if( vmap->sub_vmf_num >= sub_vmf_cap )
				{
					sub_vmf_cap = (vmap->sub_vmf_num+1) * 2;
					vmap->sub_vmfs = (RInstance_t *)realloc( vmap->sub_vmfs, sub_vmf_cap * sizeof(RInstance_t) );
				}
				
				RInstance_t *instance = vmap->sub_vmfs + (vmap->sub_vmf_num ++);
			
				int allocated = 0;
			
				// Scan for existing
				for( int i = 0; i < num_maps; i ++ )
				{
					if( tar_maps + i == vmap ) continue; // skip self to be safe
				
					// It is already existing
					if( !strcmp( subname, tar_maps[ i ].name ) )
					{
						allocated = i;
						break;
					}
				}
				
				// If not we make the allocation
				if( !allocated )
				{
					allocated = num_maps;
					_tar_loadvmf( subname );
				}
				
				instance->pVmf = tar_maps + allocated;
				vmf_bake_transform( NODE, instance->transform );

			} else return 0;
		}
	);
	
	return 1;
}

// Set the working VMF
int tar_setvmf( const char *szPath )
{
	//TODO: Free resources on tar_maps[0]. Here its memory leak!!
	num_maps = 0;
	
	return _tar_loadvmf( szPath );
}

// Add a visgroup name to push ctx's to. Returns mask bit for this group
uint32_t tar_set_group( uint32_t id, const char *szName )
{
	tar_groups[ id ] = malloc( strlen( szName ) + 1 );
	strcpy( tar_groups[ id ], szName );
	
	printf( "tar::set_group( [%u]: %s )\n", id, szName );
	
	// TODO: range check ( MAX: 32 channels )
	return 0x1 << id;
}

// Also visgroup ids
void tar_alloc_contexts(void)
{
	for( int i = 0; i < num_maps; i ++ )
	{
		// Get the groupids from the VMF
		for( int j = 0; j < 32; j ++ )
		{
			if( tar_groups[ j ] )
			{
				tar_drawgroup_t *grp = tar_maps[ i ].groups + j;
			
				grp->vgroupid = vmf_getvisgroupid( tar_maps[ i ].source, tar_groups[ j ] );
				grp->models = NULL;
			}
		}
	}
}

// Generate meshes for this VMF.
void tar_gen_meshes( tar_vmf_t* vmf )
{
	printf( "tar::gen_meshes( %s )\n", vmf->name );
	vdf_node_t *world = vdf_find( vmf->source, "world" );

	solidgen_ctx_t ctx_main;
	solidgen_ctx_init( &ctx_main );

	for( int i = 0; i < 32; i ++ )
	{
		if( tar_groups[ i ] )
		{
			vmf->groups[ i ].brush_start = ctx_main.idx;
		
			VDF_ITER( world, "solid", 
				vdf_node_t *editor = VDF( NODE, "editor" );
				if( vdfreadi( editor, "visgroupid", -2 ) == vmf->groups[ i ].vgroupid )
				{
					if( solidgen_push( &ctx_main, NODE ) == k_ESolidResult_errnomem )
					{
						fprintf( stderr, "\t@tar::gen_meshes::world[%i]: out of memory\n", i );
						abort();
					}
				}
			);
			
			VDF_ITER( vmf->source, "entity",
				vdf_node_t *editor = VDF( NODE, "editor" );
				if( vdfreadi( editor, "visgroupid", -2 ) == vmf->groups[ i ].vgroupid )
				{
					vdf_node_t *isolid = VDF( NODE, "solid" );
					if( isolid )
					{
						if( solidgen_push( &ctx_main, isolid ) == k_ESolidResult_errnomem )
						{
							fprintf( stderr, "\t@tar::gen_meshes::entities[%i]: out of memory\n", i );
							abort();
						}
					}
				}
			);
			
			vmf->groups[ i ].brush_num = ctx_main.idx - vmf->groups[ i ].brush_start;
		}
	}
	
	// TODO: push to opengl before free..
	solidgen_gl_buffer( &ctx_main, &vmf->gl.vao_brushes, &vmf->gl.vbo_brushes, &vmf->gl.ebo_brushes );
	solidgen_ctx_free( &ctx_main );
}

// Pre-cache models found in VMF file
void tar_precache_models( tar_vmf_t *vmf )
{
	// Again, issue with sb_push and mat4..
	for( int i = 0; i < 32; i ++ )
	{
		vmf->groups[ i ].model_num = 0;
		vmf->groups[ i ].model_cap = 0;
		vmf->groups[ i ].models = NULL;
	}
	
	VDF_ITER( vmf->source, "entity",
		//vdf_node_t *editor = VDF( NODE, "editor" );
		int id = vdfreadi( VDF( NODE, "editor" ), "visgroupid", -2 );
		
		for( int i = 0; i < 32; i ++ )
		{
			if( tar_groups[ i ] )
			{
				tar_drawgroup_t *grp = vmf->groups + i;
			
				if( grp->vgroupid == id )
				{
					const char *mdlName = vdf_kv_get( NODE, "model", NULL );
					
					if( mdlName )
					{
						if( grp->model_num >= grp->model_cap )
						{
							grp->model_cap = (grp->model_num+1)*2;
							grp->models = (RModel_t *)realloc( grp->models, grp->model_cap * sizeof( RModel_t ));
						}
						
						RModel_t *mdl = grp->models + (grp->model_num ++);
						
						mdl->modelid = _tar_loadmodel( mdlName );
						vmf_bake_transform( NODE, mdl->transform );
					}
				}
			}
		}
	);
}

mat4 tar_projection;

GLuint 	screen_quad_vao;
GLuint 	screen_quad_vbo;

// Flush the waiting commands
int tar_bake(void)
{
	// First time setup
	static int bake_first_time = 1;
	if( bake_first_time )
	{
		float quad_data[] = {
			-1.f, -1.f, 0.f, 0.f,
			-1.f,  1.f, 0.f, 1.f,
			 1.f,  1.f, 1.f, 1.f,
			-1.f, -1.f, 0.f, 0.f,
			 1.f,  1.f, 1.f, 1.f,
			 1.f, -1.f, 1.f, 0.f
		};
		
		glGenVertexArrays( 1, &screen_quad_vao );
		glGenBuffers( 1, &screen_quad_vbo );
		glBindVertexArray( screen_quad_vao );
			
		glBindBuffer( GL_ARRAY_BUFFER, screen_quad_vbo );
		glBufferData( GL_ARRAY_BUFFER, 24 * sizeof( float ), quad_data, GL_STATIC_DRAW );
		
		// aPos
		glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void *)0 );
		glEnableVertexAttribArray( 0 );
		
		bake_first_time = 0;
	}

	tar_alloc_contexts();
	
	for( int i = 0; i < num_maps; i ++ )
	{
		tar_gen_meshes( tar_maps + i );
		tar_precache_models( tar_maps + i );
	}
	
	glm_ortho(-2048.f, 2048.f, -2048.f, 2048.f, -10000.f, 10000.f, tar_projection );
	
	mat4 correction = GLM_MAT4_IDENTITY_INIT;	// World flipping
	
	printf( "tar::bake: complete\n" );	
	return 1;
}

void tar_drawquad(void)
{
	glBindVertexArray( screen_quad_vao );
	glDrawArrays( GL_TRIANGLES, 0, 6 );
}

GLint _uniform_projection;		// Just for rendering, eg; flip world to face camera..
GLint _uniform_transform;		// Main transform component, world full affine transform stack

// Setup shader resources from shadername
int tar_setshader( GLuint shadername )
{
	printf( "tar::setshader( %u )\n", shadername );
	// Scrape some shader details,, matrices etc
	glUseProgram( shadername );
	_uniform_projection = 	glGetUniformLocation( shadername, "in_Projection" );
	_uniform_transform = 	glGetUniformLocation( shadername, "in_Transform" );
	
	if( _uniform_projection == -1 || _uniform_transform == -1 )
	{
		fprintf( stderr, "Missing required uniform(s) from shader[ %u ]. [ in_projection, in_transform ]\n", shadername );
		return 0;
	}
	
	// Set perspective
	glUniformMatrix4fv( _uniform_projection, 1, GL_FALSE, (float *)tar_projection );
	
	return 1;
}

void _tar_rendergroup( tar_vmf_t *self, int const group )
{
	glUniformMatrix4fv( _uniform_transform, 1, GL_FALSE, (float *)self->mCurrent ); 
	
	// Do the draw the BRUSHES
	glBindVertexArray( self->gl.vao_brushes );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, self->gl.ebo_brushes ); //todo: check if this is needed
	
	glDrawElements(
		GL_TRIANGLES,
		self->groups[group].brush_num,
		GL_UNSIGNED_INT,
		(void *)( self->groups[group].brush_start * sizeof( uint32_t ))
	);
	
	// Draw the models
	mat4 modelm;
	for( int i = 0; i < self->groups[ group ].model_num; i ++ )
	{
		RModel_t *mdl = self->groups[ group ].models + i;
		glm_mat4_mul( (float(*)[4])self->mCurrent, (float(*)[4])mdl->transform, modelm );
		glUniformMatrix4fv( _uniform_transform, 1, GL_FALSE, (float *)modelm );
		
		tar_model_t *gldata = tar_models + mdl->modelid;
		
		glBindVertexArray( gldata->vao );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gldata->ebo );
		
		// Draw model
		glDrawElements(
			GL_TRIANGLES,
			gldata->idx_count,
			GL_UNSIGNED_SHORT,
			(void*)0
		);
	}

	printf( "[%i] Render vmf: %s\n", group, self->name );

	for( int i = 0; i < self->sub_vmf_num; i ++ )
	{
		RInstance_t *next = self->sub_vmfs + i;
		
		glm_mat4_mul( self->mCurrent, next->transform, next->pVmf->mCurrent );	
		_tar_rendergroup( next->pVmf, group );
	}
}

void tar_renderfiltered( uint32_t const mask )
{
	for( int i = 0; i < 32; i ++ )
	{
		if( 0x1 & ( mask >> i ) && tar_groups[ i ] )
		{
			_tar_rendergroup( tar_maps, i );
		}
	}
}

