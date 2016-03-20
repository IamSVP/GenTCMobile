//
// Created by psrihariv on 3/4/2016.
//

#ifndef GENTCMOBILE_GENTCJNICS_H
#define GENTCMOBILE_GENTCJNICS_H

#include <android/log.h>
#include <math.h>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <numeric>

#include <GLES3/gl31.h>
#include <EGL/egl.h>
#include <vector>
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include "ObjLoader/objloader.hpp"
#include "ObjLoader/vboindexer.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "crn_decomp.h"

#define DEBUG

#define LOG_TAG "GenTCJNI"
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#ifdef  DEBUG
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#endif


#ifdef DEBUG

typedef unsigned long long ull;

static const char* getGLErrString(GLenum err){

    const char *errString = "Unknown error";
    switch(err){
        case GL_INVALID_ENUM: errString = "Invalid enum"; break;
        case GL_INVALID_VALUE: errString = "Invalid value"; break;
        case GL_INVALID_OPERATION: errString = "Invalid operation"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: errString = "Invalid Frame Buffer Operation"; break;
        case GL_OUT_OF_MEMORY: errString = "Out of Memory Operation"; break;
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



class RendererCS{

public:
    ~RendererCS();
    void render();
    void init(const char *path);
    RendererCS();

    void initializeTexture();
    void initializeScene();
    void intializeShaderBuffers();
    void initializeCompressedTexture();

    void loadShaders(const char *VertexShader, const char *FragmentShader);
    void loadComputeShader(const char *ComputeShader, GLuint &computeId);
    void loadTextureDataJPG(int img_num);
    void loadTextureDataDXT(int img_num);
    void loadTextureDataCRN(int img_num);
    void loadTextureDataPBO(const char *imgPath);
    void resize(int w, int h);
    void draw();
    void drawDXT();
    GLint posLoc;
    GLint uvLoc;
    GLint texLoc;
    GLint texLocInCS;
    GLint texLocOutCS;
    GLint matrixID, viewMatrixID, modelMatrixID;
    GLuint m_ProgramId; // program ID returned after compiling the shaders
    GLuint m_ComputeId;
    GLuint m_ComputeId2;
    GLuint m_VertexBuffer; // VertexBuffer ID
    GLuint m_UVBuffer;// UV Buffer ID
    GLuint m_NormalBuffer;//Normal Buffer ID
    GLuint m_IndexBuffer;// Index Buffer ID
    GLuint m_TextureId;// Texture Id after GLgentextures
    GLuint m_TextureId2;
    GLuint m_TextureIdDXT;
    GLuint m_ssbo;
    GLuint m_VertexArrayId; // Vertex array ID
    GLbyte *m_TextureDataPtr;// Texture data pointer, data loaded from a file generally
    GLuint m_PboId[2]; // Pixel buffer objects Id
    uint32_t m_NumIndices; // number of indices
    uint32_t m_NumVertices;
    uint32_t m_TextureNumber; // number of the current Texture
    glm::mat4 m_Projection;
    glm::mat4 m_View;
    glm::mat4 m_Scaling;
    glm::mat4 m_Model;
    glm::mat4 m_MVP;
    uint32_t m_screenW;
    uint32_t m_screenH;

    uint32_t m_numframes;
    std::vector<ull> m_CPULoad;
    std::vector<ull> m_CPUDecode;
    std::vector<ull> m_GPULoad;
    std::vector<ull> m_GPUDecode;
    std::vector<ull> m_TotalFps;


    glm::vec3 m_camPosition;
    float scale[100];
    char m_TexturePath[256];
    char m_ObjPath[256];

    const EGLContext m_EglContext;

};

#endif //GENTCMOBILE_GENTCJNICS_H
