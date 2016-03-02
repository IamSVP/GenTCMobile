//
// Created by psrihariv on 3/1/2016.
//

#ifndef __GENTCMOBILE_GENTCJNI_H
#define __GENTCMOBILE_GENTCJNI_H

#include <android/log.h>
#include <math.h>
#include <cstring>
#include <cassert>



#include <GLES3/gl3.h>
#include <EGL/egl.h> /// chance of error
#include <vector>

#define DEBUG

#define LOG_TAG "GenTCJNI"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#ifdef  DEBUG
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#endif


#ifdef DEBUG

static const char* getGLErrString(GLenum err){

    const char *errString = "Unknown error";
    switch(err){
        case GL_INVALID_ENUM: errString = "Invalid enum"; break;
        case GL_INVALID_VALUE: errString = "Invalid value"; break;
        case GL_INVALID_OPERATION: errString = "Invalid operation"; break;
    }
    return errString;
}


#define CHECK_GL(fn, ...) fn(__VA_ARGS__);                                           \
  do {                                                                               \
    GLenum glErr = glGetError();                                                     \
    if(glErr != GL_NO_ERROR) {                                                       \
      const char *errString = getGLErrString(glErr);                                 \
      if(NULL != errString){                                                         \
        ALOGE("OpenGL call Error (%s : %d): %s\n",__FILE__, __LINE__, errString);    \
      } else {                                                                       \
        ALOGE("Unknown OpenGL call Error(%s : %d): %s\n", __FILE__);                 \
       }                                                                             \
      assert(false);                                                                 \
    }                                                                                \
  } while(0)
#else
#define CHECK_GL(fn, ...) do { fn(__VA__ARGS__); } while(0)
#endif

typedef struct _Vertex {

    GLfloat pos[3];
    GLfloat uv[2];
}Vertex;

class Renderer{

public:
    ~Renderer();
    void render();
    void init(const char *path);
    Renderer();

    void initializeTexture();
    void initializeCompressedTexture();
    void loadShaders(const char *VertexShader, const char *FragmentShader);
    void loadTextureDataJPG(const char *imgPath);
    void loadTextureDataPBO(const char *imgPath);


    GLuint m_ProgramId; // program ID returned after compiling the shaders
    GLuint m_VertexBuffer; // VertexBuffer ID
    GLuint m_UVBuffer;// UV Buffer ID
    GLuint m_NormalBuffer;//Normal Buffer ID
    GLuint m_IndexBuffer;// Index Buffer ID
    GLuint m_TextureId;// Texture Id after GLgentextures
    GLuint m_VertexAarryId; // Vertex array ID
    GLbyte *m_TextureDataPtr;// Texture data pointer, data loaded from a file generally
    GLuint m_pboId[2]; // Pixel buffer objects Id
    uint32_t m_NumIndices; // number of indices
    uint32_t m_TextureNumber; // number of the current Texture
    char m_TexturePath[256];
    const EGLContext m_EglContext;




};
#endif //GENTCMOBILE_GENTCJNI_H
