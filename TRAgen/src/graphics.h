#ifndef GRAPHICS_H
#define GRAPHICS_H


/**
 * Just inits the OpenGL stuff...
 */
bool initializeGL(void);

/**
 * Just handles the idle loops... the OpenGL stuff...
 */
void loopGL(void);

/**
 * Just closes the OpenGL stuff...
 */
void closeGL(void);


#ifdef RENDER_TO_IMAGES
/**
 * Just inits the files to render into...
 */
void initiateImageFile(void);
#endif

#endif
