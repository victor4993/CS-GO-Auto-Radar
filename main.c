#include "common.h"
#include "time.h"
#include "tar_main.h"
#include "cmdseq.h"

GLFWwindow* g_hWindowMain;
float		g_fTime;
float		g_fTimeLast;
float		g_fTimeDelta;
int 		g_nWindowW;
int 		g_nWindowH;

// OPenGL error callback (4.5+)
void APIENTRY openglCallbackFunction(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam) {
	if (type == GL_DEBUG_TYPE_OTHER) return; // We dont want general openGL spam.

	fprintf( stderr, "OpenGL message: %s\n", message);

	uint32_t bShutdown = 0;

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		fprintf( stderr,"Type: ERROR\n"); 
		bShutdown = 1;
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		fprintf( stderr,"Type: DEPRECATED_BEHAVIOR\n"); break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		fprintf( stderr,"Type: UNDEFINED_BEHAVIOR\n"); break;
	case GL_DEBUG_TYPE_PORTABILITY:
		fprintf( stderr,"Type: PORTABILITY\n"); break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		fprintf( stderr,"Type: PERFORMANCE\n"); break;
	case GL_DEBUG_TYPE_OTHER:
		fprintf( stderr,"Type: OTHER\n"); break;
	}

	fprintf( stderr,"ID: %u\n", id);
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:
		fprintf( stderr,"Severity: LOW\n"); break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		fprintf( stderr,"Severity: MEDIUM\n"); break;
	case GL_DEBUG_SEVERITY_HIGH:
		fprintf( stderr,"Severity: HIGH\n"); 
		bShutdown = 1;
		break;
	}
	
	if(bShutdown) glfwSetWindowShouldClose( g_hWindowMain, 1 );
}

int engine_init(void)
{
	printf( "engine::init()\n" );

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
	
	g_hWindowMain = glfwCreateWindow(1024, 1024, "tar3", NULL, NULL);
	
	if(!g_hWindowMain){
		fprintf( stderr, "GLFW Failed to initialize\n");
		return 0;
	}
	
	glfwMakeContextCurrent(g_hWindowMain);
	
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf( stderr, "Glad failed to initialize\n");
		return 0;
	}

	const unsigned char* glver = glGetString(GL_VERSION);
	printf("Load setup complete, OpenGL version: %s\n", glver);

	if (glDebugMessageCallback) {
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openglCallbackFunction, NULL);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE,
			GL_DONT_CARE,
			GL_DONT_CARE,
			0,
			&unusedIds,
			GL_TRUE);
	}
	else {
		printf("glDebugMessageCallback not availible\n");
	}
	
	return 1;
}

void engine_quit(void)
{
	glfwTerminate();
}


inline __attribute__((always_inline))
void engine_newframe(void)
{
	glfwPollEvents();

	g_fTimeLast = g_fTime;
	g_fTime = glfwGetTime();
	g_fTimeDelta = __min(g_fTime - g_fTimeLast, 0.1f);
	
	glfwGetFramebufferSize(g_hWindowMain, &g_nWindowW, &g_nWindowH);
	glViewport(0, 0, g_nWindowW, g_nWindowH);

	// Drawing code
	glClearColor(.05f, .05f, .05f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
}

inline __attribute__((always_inline))
void engine_endframe(void)
{
	glfwSwapBuffers(g_hWindowMain);
}

typedef struct {
	char 	test[30];
	mat4	yuh;
} test_t;

test_t *buffer[16];

void fbuffer_reset(void)
{
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, g_nWindowW, g_nWindowH );
}

int main(void)
{
	// OpenGL context / windowing initialization
	if( !engine_init() )
	{
		engine_quit();
		return 0;
	}

	// TODO: make function of commandline args
	con_setup();
	cmdseq_reg();
	
	engine_newframe();
	
	con_run_cfg( "scripts/main.rat" );
	
	// Shutdown
	engine_quit();
	fs_exit();
	printf( "tar::exit()\n" );
	
	return 0;
	

	// Gameinfo needs to be set to the main CS:GO installation so it can open vpks and shit. it will open the searchpaths automatically
	fs_set_gameinfo( "/home/harry/SteamLibrary/steamapps/common/Counter-Strike Global Offensive/csgo/gameinfo.txt" );
	
	// tar_push_group( <name> ) flags up certain groups to be tracked when building
	// geometry. Each group registered here, when the vmf is loaded, will create an index 
	// buffer for that respective group so it can be drawn in one go.
	// 
	// Models and brushes and other brush entities are treated the exact same way as eachother
	//
	// These 'id's are bitmasks. So grp_layout gets: ...0001 and overlap ...0010

	//uint32_t grp_layout = tar_push_group( "tar_layout" );
	//uint32_t grp_overlap = tar_push_group( "tar_cover" );
	
	// tar_setvmf will load up the vmf and any instances. no processing is done yet, except
	// for baking transforms of instance entities
	if( !tar_setvmf( "testmap.vmf" ) )
	{
		return 0;
	}
	
	// Starts and clears context ( needs clean-up since this is old Voyager code )
	engine_newframe();
	
	// Compile shaders here
	GLuint shader_gbuffer = shader_compile( "shaders/base.vs", "shaders/gbuffer.fs" );
	GLuint shader_screentex = shader_compile( "shaders/screen.vs", "shaders/screen_texture.fs" );
	
	// tar_bake builds vmf geometry into drawable stuff
	tar_bake();
	
	// Main GBuffer
	gbuffer_t fb_layout;
	gbuffer_init( &fb_layout, 1024, 1024 );
	gbuffer_use( &fb_layout );
	
	// Frame clear
	glClearColor( 0.f, 0.f, 0.f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	glEnable( GL_DEPTH_TEST );
	
	// Renderfiltered 0xFFFFFFF means draw everything basically. 
	// If you want specific layer (or layers for example: tar_renderfiltered( grp_layout | grp_overlap );
	tar_setshader( shader_gbuffer );
	tar_renderfiltered( 0xFFFFFFFF );
	
	// Reset framebuffer back to primary window's one
	fbuffer_reset();
	glDisable(GL_DEPTH_TEST);
	

	// This section is just outputting stuff to files
	// =========================================================================
	glUseProgram( shader_screentex );
	glActiveTexture( GL_TEXTURE0 );
	glUniform1i( glGetUniformLocation( shader_screentex, "in_Sampler" ), 0 );
	
	// Render position buffer
	glBindTexture( GL_TEXTURE_2D, fb_layout.texPosition );
	tar_drawquad();
	render_to_png_flat( 1024, 1024, "test_pos.png" );
	
	// Normal buffer
	glBindTexture( GL_TEXTURE_2D, fb_layout.texNormal );
	tar_drawquad();
	render_to_png_flat( 1024, 1024, "test_normal.png" );
	
	// Origin buffer
	glBindTexture( GL_TEXTURE_2D, fb_layout.texOrigin );
	tar_drawquad();
	render_to_png_flat( 1024, 1024, "test_origin.png" );
	
	// =========================================================================
	
	return 0;
}
