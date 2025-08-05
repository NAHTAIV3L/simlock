
#include "./window.h"
#include "./state.h"
#include "./glad/glad.h"

extern PFNEGLCREATEPLATFORMWINDOWSURFACEPROC eglCreatePlatformWindowSurfaceEXT;

void window_redraw(window_t *win) {
    eglMakeCurrent(win->state->egl_display, win->egl_surface, win->egl_surface, win->state->egl_context);
    glViewport(0, 0, win->width, win->height);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_POLYGON);
    glColor3f(1, 0, 0);
    glVertex3f(-0.5, -0.5, 0);

    glColor3f(0, 1, 0);
    glVertex3f(0.5, -0.5, 0);

    glColor3f(0, 0, 1);
    glVertex3f(0, 0.5, 0);
    glEnd();

    eglSwapBuffers(win->state->egl_display, win->egl_surface);
    eglMakeCurrent(win->state->egl_display, NULL, NULL, win->state->egl_context);
}
