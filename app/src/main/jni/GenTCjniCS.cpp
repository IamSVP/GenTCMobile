//
// Created by psrihariv on 3/4/2016.
//

#include "GenTCjniCS.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <jni.h>
#include <cstdlib>

#define SPHERE

const char *vVertexProg =
        "#version 310 es\n"
                "precision mediump float;\n"
                "in vec3 position;\n"
                "in vec2 texCoord;\n"
                "uniform mat4 MVP;\n"
                "out vec2 uv;\n"
                "void main() {\n"
                "  gl_Position = MVP*vec4(position, 1.0);\n"
                "  uv = texCoord;\n"
                "}\n";

const char *vFragProg =
        "#version 310 es\n"
                "precision mediump float;\n"
                "in vec2 uv;\n"
                "uniform sampler2D tex;\n"
                "out vec4 color;\n"
                "void main() {\n"
                "  color = vec4(texture(tex, uv).rgb, 1);\n"
                "}\n";
const char *vComputeProg =
        "#version 310 es\n"
                "precision mediump float;\n"
                "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1)in;\n"
                "layout(std430) buffer; // Sets the default layout for SSBOs.\n"
                "layout(binding = 0) buffer Data {\n"
                "float data[]; // This array can now be tightly packed.\n"
                "};"
                "layout(binding = 1, rgba8ui) uniform readonly uimage2D colImage;\n"
                "layout(binding = 2, rgba8ui) uniform writeonly uimage2D bwImage;\n"
                "void main() {\n"
                "uvec4 color = imageLoad(colImage, ivec2(gl_GlobalInvocationID.xy));\n"
                "float s = data[int(mod(float(gl_GlobalInvocationID.x), float(100)))];\n"
                "imageStore(bwImage, ivec2(gl_GlobalInvocationID.xy), uvec4(uint(s*float(color.r)),uint(s*float(color.g)),uint(s*float(color.b)),1));\n"
                "memoryBarrierImage();\n"
                "barrier();\n"
                "}\n";


static const int vImageWidth = 1920;
static const int vImageHeight = 1080;

static void printGlString(const char* name, GLenum s) {
    const char* v = (const char*)glGetString(s);
    ALOGV("GL %s: %s\n", name, v);
}



void RendererCS::loadShaders(const char *VertexShader, const char *FragmentShader){

  // Get Id's for the create shaders
  GLuint verShdrId = CHECK_GL(glCreateShader,GL_VERTEX_SHADER );
  GLuint fragShdrId = CHECK_GL(glCreateShader, GL_FRAGMENT_SHADER);

  // Give the Shader Source and give the Id to compile
  CHECK_GL(glShaderSource,verShdrId, 1, &vVertexProg, NULL );
  CHECK_GL(glCompileShader, verShdrId);

  // check vertex shader

  int result, logLength;
  CHECK_GL(glGetShaderiv, verShdrId, GL_COMPILE_STATUS, &result);

  if(result != GL_TRUE){
    CHECK_GL(glGetShaderiv, verShdrId, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> VertexShaderErrorMsg(logLength);
    CHECK_GL(glGetShaderInfoLog, verShdrId, logLength, NULL, &VertexShaderErrorMsg[0]);
    ALOGE("Error while compiling vertex shader", &VertexShaderErrorMsg[0]);
    exit(1);
  }

  // create the fragment shader
  CHECK_GL(glShaderSource, fragShdrId, 1, &vFragProg, NULL);
  CHECK_GL(glCompileShader, fragShdrId);
  CHECK_GL(glGetShaderiv, fragShdrId, GL_COMPILE_STATUS, &result);

  if(result != GL_TRUE){
    CHECK_GL(glGetShaderiv, fragShdrId, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> FragmentShaderErrMsg(logLength);
    CHECK_GL(glGetShaderInfoLog, fragShdrId, logLength, NULL, &FragmentShaderErrMsg[0]);
    ALOGE("Error while comipling fragment shader", &FragmentShaderErrMsg[0]);
    exit(1);
  }


  m_ProgramId = CHECK_GL(glCreateProgram);
  CHECK_GL(glAttachShader, m_ProgramId, verShdrId);
  CHECK_GL(glAttachShader, m_ProgramId, fragShdrId);
  CHECK_GL(glLinkProgram, m_ProgramId);

  CHECK_GL(glGetProgramiv, m_ProgramId, GL_LINK_STATUS, &result);
  if(result != GL_TRUE){
    CHECK_GL(glGetProgramiv, m_ProgramId, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> ProgramErrMsg(logLength);
    CHECK_GL(glGetProgramInfoLog, m_ProgramId, logLength, NULL, &ProgramErrMsg[0]);
    ALOGE("Error while Linking the program", &ProgramErrMsg[0]);
  }



}

// Look up more on gl_context and egl context and how's it handled
RendererCS::RendererCS():m_EglContext(eglGetCurrentContext())
{
}

void RendererCS::intializeShaderBuffers(){

  CHECK_GL(glGenBuffers, 1, &m_ssbo);

}
void RendererCS::initializeTexture(){

  CHECK_GL(glGenTextures, 1, &m_TextureId);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureId);
 // CHECK_GL(glTexImage2D, GL_TEXTURE_2D,0,GL_RGBA8UI, vImageWidth, vImageHeight,0,GL_RGBA_INTEGER,GL_UNSIGNED_BYTE,NULL);

  CHECK_GL(glTexStorage2D, GL_TEXTURE_2D, 1, GL_RGBA8,vImageWidth, vImageHeight );
  CHECK_GL(glTexParameteri,GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  CHECK_GL(glBindImageTexture, 2, m_TextureId, 0, GL_FALSE,0, GL_WRITE_ONLY, GL_RGBA8UI); // possible error
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);




  CHECK_GL(glGenTextures, 1, &m_TextureId2);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureId2);
  CHECK_GL(glTexStorage2D, GL_TEXTURE_2D, 1, GL_RGBA8,vImageWidth, vImageHeight );
  //CHECK_GL(glTexImage2D, GL_TEXTURE_2D,0,GL_RGBA8UI, vImageWidth, vImageHeight,0,GL_RGBA_INTEGER,GL_UNSIGNED_BYTE,NULL);
  CHECK_GL(glTexParameteri,GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  CHECK_GL(glBindImageTexture, 1, m_TextureId2, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);

  CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);




    CHECK_GL(glGenBuffers, 2, m_PboId);
    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_PboId[0]);
    CHECK_GL(glBufferData, GL_PIXEL_UNPACK_BUFFER, vImageHeight*vImageWidth * 4, NULL, GL_DYNAMIC_DRAW);
    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_PboId[1]);
    CHECK_GL(glBufferData, GL_PIXEL_UNPACK_BUFFER, vImageHeight*vImageWidth * 4, NULL, GL_DYNAMIC_DRAW);
    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
}

void RendererCS::initializeScene() {

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    bool res = loadOBJ(m_ObjPath, vertices, uvs, normals);
    ALOGE("object path....%s\n", m_ObjPath);
    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> indexed_uvs;
    std::vector<glm::vec3> indexed_normals;
    indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

    CHECK_GL(glGenBuffers, 1, &m_VertexBuffer);
    CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, m_VertexBuffer);
    CHECK_GL(glBufferData,GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

    CHECK_GL(glGenBuffers, 1, &m_UVBuffer);
    CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, m_UVBuffer);
    CHECK_GL(glBufferData, GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);


    // Generate a buffer for the indices as well
    CHECK_GL(glGenBuffers, 1, &m_IndexBuffer);
    CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
    CHECK_GL(glBufferData, GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);
    m_NumIndices = indices.size();
    ALOGE("Indices size %d\n", m_NumIndices);




}
RendererCS::~RendererCS(){

    CHECK_GL(glDeleteBuffers, 1, &m_VertexBuffer);
    CHECK_GL(glDeleteBuffers, 1, &m_UVBuffer);
    CHECK_GL(glDeleteBuffers, 1, &m_IndexBuffer);
    CHECK_GL(glDeleteProgram, m_ProgramId);
    CHECK_GL(glDeleteTextures, 1, &m_TextureId);
    CHECK_GL(glDeleteTextures,1, &m_TextureId2);
    CHECK_GL(glDeleteBuffers, 2, m_PboId);
    CHECK_GL(glDeleteVertexArrays, 1, &m_VertexArrayId);

}


void RendererCS::init(const char * path){

    sprintf(m_TexturePath,"%s/Textures",path);
    sprintf(m_ObjPath, "%s/Obj/cube.obj", path);
    ALOGE("Path obj %s\n", m_ObjPath);
    m_TextureNumber = 1;
    CHECK_GL(glGenVertexArrays, 1, &m_VertexArrayId);
    CHECK_GL(glBindVertexArray, m_VertexArrayId);
    // compile shaders
    for(int i=0; i<100; i++)
      scale[i] = 1.0;
    CHECK_GL(glGenBuffers, 1, &m_ssbo);


    loadShaders(vVertexProg, vFragProg);
    loadComputeShader(vComputeProg);



    // create the quad screen

#ifndef SPHERE
    static const GLfloat fullscreen[] =  {
        -1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,


    };
//    glGenBuffers(1, &m_VertexBuffer);
    CHECK_GL(glGenBuffers, 1, &m_VertexBuffer);
    CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, m_VertexBuffer);
    CHECK_GL(glBufferData, GL_ARRAY_BUFFER,sizeof(fullscreen), fullscreen, GL_STATIC_DRAW);
    CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, 0);



    static const GLfloat UVscreen[] = {
          0.0f, 0.0f,
          1.0f, 0.0f,
          1.0f, 1.0f,
          0.0f, 1.0f
    };

    CHECK_GL(glGenBuffers, 1, &m_UVBuffer);
    CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, m_UVBuffer);
    CHECK_GL(glBufferData, GL_ARRAY_BUFFER, sizeof(UVscreen), UVscreen, GL_STATIC_DRAW);
    CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER,0);
    static const unsigned short Indices[] = {
           0,2,1,
           0,3,2
    };
    CHECK_GL(glGenBuffers, 1, &m_IndexBuffer);
    CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
    CHECK_GL(glBufferData, GL_ELEMENT_ARRAY_BUFFER,sizeof(Indices), Indices,GL_STATIC_DRAW);
    CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER,0);
    m_NumIndices = 6;
#endif

#ifdef SPHERE

    initializeScene();
    m_Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    m_View = glm::lookAt(
                            glm::vec3(4,3,4),
                            glm::vec3(0,0,0),
                            glm::vec3(0,1,0));
    m_MVP = m_Projection * m_View;


#endif
    initializeTexture();

    posLoc = CHECK_GL(glGetAttribLocation,m_ProgramId, "position");
    assert ( posLoc >= 0 );

    uvLoc = CHECK_GL(glGetAttribLocation, m_ProgramId, "texCoord");
    assert ( uvLoc >= 0 );

    texLoc = CHECK_GL(glGetUniformLocation, m_ProgramId, "tex");
    assert ( texLoc >= 0 );

    texLocInCS = CHECK_GL(glGetUniformLocation, m_ComputeId, "colImage");
    assert(texLocInCS >=0 );
    texLocOutCS = CHECK_GL(glGetUniformLocation, m_ComputeId, "bwImage");
    assert(texLocOutCS >= 0);

    matrixID = CHECK_GL(glGetUniformLocation, m_ProgramId, "MVP");
    assert(matrixID >= 0);


}

void RendererCS::loadTextureDataJPG(int img_num) {

  char img_path[256];
  sprintf(img_path, "%s/frame%04d.jpg",m_TexturePath,img_num);
    ALOGE("Path%s", m_TexturePath);
    int w,h,n;
  unsigned char *TextureData= stbi_load(img_path, &w, &h, &n, 4);
  ALOGE("checking the loaded texture: %d %d %d \n", w, h, img_num);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureId2);

  CHECK_GL(glTexSubImage2D, GL_TEXTURE_2D,
           0,
           0,0,
           vImageWidth,
           vImageHeight,
           GL_RGBA,
           GL_UNSIGNED_BYTE,
           TextureData);
    CHECK_GL(glBindTexture,GL_TEXTURE_2D, 0);
 stbi_image_free(TextureData);

}
void RendererCS::resize(int w, int h){

    CHECK_GL(glViewport, 0,0,w,h);
    ALOGE("view port size: %d %d", w,h);

}

void RendererCS::loadComputeShader(const char *ComputeShader){

    GLuint compShdrId = CHECK_GL(glCreateShader, GL_COMPUTE_SHADER);
    CHECK_GL(glShaderSource, compShdrId, 1, &vComputeProg, NULL);
    CHECK_GL(glCompileShader, compShdrId);

    int result, logLength;
    CHECK_GL(glGetShaderiv, compShdrId, GL_COMPILE_STATUS, &result);

    if(result != GL_TRUE){

        CHECK_GL(glGetShaderiv, compShdrId, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> ComputeShaderErrorMsg(logLength);
        CHECK_GL(glGetShaderInfoLog, compShdrId, logLength, NULL, &ComputeShaderErrorMsg[0]);
        ALOGE("Error while compiling compute shader", &ComputeShaderErrorMsg[0]);
        exit(1);
    }

    m_ComputeId = CHECK_GL(glCreateProgram);
    CHECK_GL(glAttachShader, m_ComputeId, compShdrId);
    CHECK_GL(glLinkProgram, m_ComputeId);


    CHECK_GL(glGetProgramiv, m_ComputeId, GL_LINK_STATUS, &result);
    if(result != GL_TRUE){

        CHECK_GL(glGetProgramiv, m_ComputeId, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> ProgramErrMsg(logLength);
        glGetProgramInfoLog(m_ComputeId, logLength, NULL, &ProgramErrMsg[0]);
        ALOGE("Error while Linking porgram", &ProgramErrMsg[0]);
        exit(1);

    }




}


void RendererCS::draw(){


  CHECK_GL(glUseProgram,m_ComputeId);
  // Let's do some error checking
  int result;

  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();



  CHECK_GL(glGetProgramiv,m_ComputeId, GL_ACTIVE_UNIFORMS, &result);
  ALOGE("count of active uniforms%d",result);

// buffer for name
    GLchar * name = new GLchar[200];
// buffer for length
    GLsizei length = 100;
// Type of variable
    GLenum type;
// Count of variables
    GLint size;
// loop over active uniforms getting each's name


  CHECK_GL(glGetActiveUniform, m_ComputeId, 0,200,&length,&size,&type, name);
  ALOGE("index location%d\n", texLocInCS);
  ALOGE("index location 2 %d\n", texLocOutCS);

  m_TextureNumber = m_TextureNumber % 20 + 1;
  loadTextureDataJPG(m_TextureNumber);


  std::chrono::high_resolution_clock::time_point compute_start = std::chrono::high_resolution_clock::now();
  CHECK_GL(glBindBufferBase,GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
  CHECK_GL(glBufferData, GL_SHADER_STORAGE_BUFFER, sizeof(scale), scale, GL_DYNAMIC_COPY);
  CHECK_GL(glDispatchCompute, 1920/8, 1080/8, 1);
  CHECK_GL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  std::chrono::high_resolution_clock::time_point compute_end = std::chrono::high_resolution_clock::now();
  std::chrono::nanoseconds compute_time = std::chrono::duration_cast<std::chrono::nanoseconds>(compute_end - compute_start);
  ALOGE("Compute Time %lld\n", compute_time.count());


  CHECK_GL(glUseProgram,m_ProgramId);
    CHECK_GL(glClear,GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  CHECK_GL(glUniformMatrix4fv, matrixID, 1, GL_FALSE, &m_MVP[0][0]);

  CHECK_GL(glActiveTexture, GL_TEXTURE0);


  CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureId);

  CHECK_GL(glEnableVertexAttribArray, posLoc);
  CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, m_VertexBuffer);
  CHECK_GL(glVertexAttribPointer,posLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, 0);

  CHECK_GL(glEnableVertexAttribArray,uvLoc);
  CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, m_UVBuffer);
  CHECK_GL(glVertexAttribPointer,uvLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, 0);

  CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
  CHECK_GL(glDrawElements, GL_TRIANGLES, m_NumIndices , GL_UNSIGNED_SHORT, NULL);

  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  ALOGE("Frame-Rate %lld\n", frame_time.count());

  CHECK_GL(glDisableVertexAttribArray,posLoc);
  CHECK_GL(glDisableVertexAttribArray,uvLoc);
  CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, 0);
  CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);
  CHECK_GL(glUseProgram, 0);

}





RendererCS* g_renderer = NULL;
extern "C" {
JNIEXPORT void JNICALL Java_com_example_psrihariv_gentcmobile_GenTCJNILib_init(JNIEnv* env, jobject obj, jstring path);
JNIEXPORT void JNICALL Java_com_example_psrihariv_gentcmobile_GenTCJNILib_resize(JNIEnv* env, jobject obj, jint width, jint height);
JNIEXPORT void JNICALL Java_com_example_psrihariv_gentcmobile_GenTCJNILib_draw(JNIEnv* env, jobject obj);
JNIEXPORT void JNICALL Java_com_example_psrihariv_gentcmobile_GenTCJNILib_sendString(JNIEnv* env, jobject obj, jstring path);
};

JNIEXPORT void JNICALL
Java_com_example_psrihariv_gentcmobile_GenTCJNILib_init(JNIEnv* env, jobject obj, jstring path) {
    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);
  g_renderer = new RendererCS();
    const char *Cpath = env->GetStringUTFChars(path,NULL);

    ALOGE(Cpath);
    g_renderer->init(Cpath);
}

JNIEXPORT void JNICALL
Java_com_example_psrihariv_gentcmobile_GenTCJNILib_resize(JNIEnv* env, jobject obj, jint width, jint height) {
    if (g_renderer) {
        g_renderer->resize(width, height);
    }
}

JNIEXPORT void JNICALL
Java_com_example_psrihariv_gentcmobile_GenTCJNILib_draw(JNIEnv* env, jobject obj) {
    if (g_renderer) {
        g_renderer->draw();
    }
}
