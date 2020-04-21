#define GL_GLEXT_PROTOTYPES why
//#include<stdio.h>
//#include<stdbool.h>
//#include<stdlib.h>
//#include<stdint.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "shader.h"
#include "sync.h"
static char* vshader = "#version 450\nvec2 y=vec2(1.,-1);vec4 x[4]={y.yyxx,y.xyxx,y.yxxx,y.xxxx};void main(){gl_Position=x[gl_VertexID];}";

#define CANVAS_WIDTH 1920
#define CANVAS_HEIGHT 1080
//#define DEBUG

GLuint vao;
GLuint p;

// <sync stuff
int curtime_ms = 0;
int is_playing = 1;
int rps = 24;
static struct sync_device *device;
#if !defined(SYNC_PLAYER)
static struct sync_cb cb;
#endif
static const char* s_trackNames[] = {"cam_x", "cam_y", "cam_z"};
static const struct sync_track* s_tracks[3];

static int row_to_ms_round(int row, float rps){
	const float newTime = (float) row / rps;
	return (floor(newTime * 1000.0f + 0.5f));
}
static float ms_to_row_f(int time_ms, float rps) {
	return rps * ((float) time_ms) * 1.0f/1000.0f;
}
static int ms_to_row_round(int time_ms, float rps){
	const float r = ms_to_row_f(time_ms, rps);
	return (int) (floor(r + 0.5f));
}
#if !defined(SYNC_PLAYER)
static void xpause(void* data, int flag)
{
	(void)data;
	if (flag)
		is_playing = 0;
	else
		is_playing = 1;
}
static void xset_row(void* data, int row)
{
	int newtime_ms = row_to_ms_round(row, rps);
	curtime_ms = newtime_ms;
	(void)data;
}
static int xis_playing(void* data)
{
	(void)data;
	return is_playing;
}
#endif
static int rocket_update(){
	if(is_playing) curtime_ms += 16;
	#if !defined( SYNC_PLAYER )
	int row = ms_to_row_round(curtime_ms, rps);
	if (sync_update(device,row,&cb,0))
		sync_tcp_connect(device, "localhost", SYNC_DEFAULT_PORT);
	#endif
	return -1;
}
// sync stuff>

static gboolean
on_render (GtkGLArea *glarea, GdkGLContext *context)
{
	glUseProgram(p);
	float row_f = ms_to_row_f(curtime_ms, rps);
	glProgramUniform1f(p, 0, curtime_ms/1000.0f);
	glProgramUniform1f(p, 1, sync_get_val(s_tracks[0], row_f));
	glProgramUniform1f(p, 2, sync_get_val(s_tracks[1], row_f));
	glProgramUniform1f(p, 3, sync_get_val(s_tracks[2], row_f));
	glBindVertexArray(vao);
	glVertexAttrib1f(0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	rocket_update();
	return TRUE;
}

static void on_realize(GtkGLArea *glarea)
{
	gtk_gl_area_make_current(glarea);
	
	device = sync_create_device("data/sync");
	for (int i = 0; i < 3; ++i)
		s_tracks[i] = sync_get_track(device, s_trackNames[i]);
	#if !defined( SYNC_PLAYER )
	sync_tcp_connect(device, "localhost", SYNC_DEFAULT_PORT);
	cb.is_playing = xis_playing;
	cb.pause = xpause;
	cb.set_row = xset_row;
	#endif

	// compile shader
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &shader_frag, NULL);
	glCompileShader(f);

	#ifdef DEBUG
		GLint isCompiled = 0;
		glGetShaderiv(f, GL_COMPILE_STATUS, &isCompiled);
		if(isCompiled == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetShaderInfoLog(f, maxLength, &maxLength, error);
			printf("%s\n", error);

			exit(-10);
		}
	#endif

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vshader, NULL);
	glCompileShader(v);

	#ifdef DEBUG
		GLint isCompiled2 = 0;
		glGetShaderiv(v, GL_COMPILE_STATUS, &isCompiled2);
		if(isCompiled2 == GL_FALSE) {
			GLint maxLength = 0;
			glGetShaderiv(v, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetShaderInfoLog(v, maxLength, &maxLength, error);
			printf("%s\n", error);

			exit(-10);
		}
	#endif

	// link shader
	p = glCreateProgram();
	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);

	#ifdef DEBUG
		GLint isLinked = 0;
		glGetProgramiv(p, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE) {
			GLint maxLength = 0;
			glGetProgramiv(p, GL_INFO_LOG_LENGTH, &maxLength);

			char* error = malloc(maxLength);
			glGetProgramInfoLog(p, maxLength, &maxLength,error);
			printf("%s\n", error);

			exit(-10);
		}
	#endif

	glProgramUniform2f(p, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
	glGenVertexArrays(1, &vao);

	// if you want to continuously render the shader once per frame
	GdkGLContext *context = gtk_gl_area_get_context(glarea);
	GdkWindow *glwindow = gdk_gl_context_get_window(context);
	GdkFrameClock *frame_clock = gdk_window_get_frame_clock(glwindow);

	// Connect update signal:
	g_signal_connect_swapped(frame_clock, "update", G_CALLBACK(gtk_gl_area_queue_render), glarea);

	// Start updating:
	gdk_frame_clock_begin_updating(frame_clock);
}

void _start() {
	asm volatile("sub $8, %rsp\n");

	typedef void (*voidWithOneParam)(int*);
	voidWithOneParam gtk_init_one_param = (voidWithOneParam)gtk_init;
	(*gtk_init_one_param)(NULL);

	GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *glarea = gtk_gl_area_new();
	gtk_container_add(GTK_CONTAINER(win), glarea);

	g_signal_connect(win, "destroy", gtk_main_quit, NULL);
	g_signal_connect(glarea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glarea, "render", G_CALLBACK(on_render), NULL);

	gtk_widget_show_all (win);

	//gtk_window_fullscreen((GtkWindow*)win);
	GdkWindow* window = gtk_widget_get_window(win);
	GdkCursor* Cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
	gdk_window_set_cursor(window, Cursor);

	gtk_main();

	asm volatile(".intel_syntax noprefix");
	asm volatile("push 231"); //exit_group
	asm volatile("pop rax");
	// asm volatile("xor edi, edi");
	asm volatile("syscall");
	asm volatile(".att_syntax prefix");
	__builtin_unreachable();
	// return 0;
}
