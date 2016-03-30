//
// Created by psrihariv on 3/4/2016.
//

#include "GenTCjniCS.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <jni.h>
#include <cstdlib>

const char *vVertexProg =
        "#version 310 es\n"
                "precision mediump float;\n"
                "in vec3 position;\n"
                "in vec2 texCoord;\n"
                "out vec2 uv;\n"
                "uniform mat4 MVP;\n"
                "void main() {\n"
                "  gl_Position =  MVP*vec4(position, 1.0);\n"
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
                "   //color = vec4(1.0, 0.0, 0.0, 1.0);\n"
                "}\n";


const char *vComputeProg =
        "#version 310 es\n"
                "precision mediump float;\n"
                "precision mediump uimage2D;\n"
                "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1)in;\n"
                "layout(std430) buffer; // Sets the default layout for SSBOs.\n"
                "layout(binding = 0) buffer Data {\n"
                "float data[]; // This array can now be tightly packed.\n"
                "};\n"
                "layout(binding = 1, rgba8ui) uniform readonly uimage2D colImage;\n"
                "layout(binding = 2, rgba8ui) uniform writeonly uimage2D bwImage;\n"
                "void main() {\n"
                "   uvec4 color = imageLoad(colImage, ivec2(gl_GlobalInvocationID.xy));\n"
                "   float s = data[uint(mod(float(gl_GlobalInvocationID.x), float(100)))];\n"
                "   imageStore(bwImage, ivec2(gl_GlobalInvocationID.xy), uvec4(uint(s*float(color.r)),uint(s*float(color.g)),uint(s*float(color.b)),1));\n"
                "   memoryBarrierImage();\n"
                "barrier();\n"
                "}\n";

const char *vDXTComputeProg =
        "#version 310 es\n"
            "precision mediump float;\n"
            "precision mediump int;\n"
            "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
            "layout(std430) buffer; // Sets the default layout for SSBO's. \n"
            "struct DXTBlock {\n"
            "uint endpoints;\n"
            "uint interpolated;\n"
            "};\n"
            "layout(binding = 0) buffer Data{\n"
            "   DXTBlock blocks[];\n"
            "};\n"
            "void main() {\n"
            "   uint color = uint(0xFFFFFFFF);\n"
            "   uint zero = uint(0);\n"
            "   blocks[uint( uint(gl_WorkGroupID.x) + uint(320) * uint(gl_WorkGroupID.y) )].endpoints = color * uint( mod(float(gl_WorkGroupID.x), float(2)) );\n "
            "   blocks[uint( uint(gl_WorkGroupID.x) + uint(320) * uint(gl_WorkGroupID.y) )].interpolated = zero ; \n"
            "   memoryBarrierBuffer(); \n"
            "   barrier(); \n"
            "}\n";


static const int vImageWidth = 2560;
static const int vImageHeight = 1280;
#define COMPRESSED_RGBA_ASTC_4x4 0x93B0
#define COMPRESSED_RGBA_ASTC_8x8 0x93B7
#define SPHERE

#define ASTC4x4
#define ASTC8x8
#define ASTC12x12
#define ASTC
#define ETC
//#define DXT1
#define CRN

static void printGlString(const char* name, GLenum s) {
    const char* v = (const char*)glGetString(s);
    ALOGE("GL %s: %s\n", name, v);
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
RendererCS::RendererCS():m_EglContext(eglGetCurrentContext()) {
}

void RendererCS::intializeShaderBuffers(){

  CHECK_GL(glGenBuffers, 1, &m_ssbo);

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
    CHECK_GL(glDeleteBuffers, 1, &m_ssbo);
    CHECK_GL(glDeleteProgram, m_ProgramId);
    CHECK_GL(glDeleteProgram, m_ComputeId);
    CHECK_GL(glDeleteProgram, m_ComputeId2);
    CHECK_GL(glDeleteTextures, 1, &m_TextureId);
    CHECK_GL(glDeleteTextures,1, &m_TextureId2);
    CHECK_GL(glDeleteTextures, 1, &m_TextureIdDXT);
    //CHECK_GL(glDeleteBuffers, 2, m_PboId);
    CHECK_GL(glDeleteVertexArrays, 1, &m_VertexArrayId);
    ALOGE("This is called...\n");

}

void RendererCS::init(const char * path){

    sprintf(m_TexturePath,"%s/Textures",path);
    sprintf(m_ObjPath, "%s/Obj/sphere.obj", path);
    sprintf(m_MetricsPath, "%s/output.txt", path);
    fpOutFile = fopen(m_MetricsPath, "r");
    ALOGE("Path obj %s\n", m_ObjPath);
    m_TextureNumber = 1;
    CHECK_GL(glGenVertexArrays, 1, &m_VertexArrayId);
    CHECK_GL(glBindVertexArray, m_VertexArrayId);
    // compile shaders
    for(int i=0; i<100; i++)
      scale[i] = 0.5;
    CHECK_GL(glGenBuffers, 1, &m_ssbo);

    CHECK_GL(glDisable, GL_CULL_FACE);
    loadShaders(vVertexProg, vFragProg);
    loadComputeShader(vComputeProg, m_ComputeId);
    loadComputeShader(vDXTComputeProg, m_ComputeId2);
    ALOGE("shader ID %d\n", m_ComputeId2);
    m_numframes = 0;



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
    m_Projection = glm::perspective(90.0f, 1.0f, 0.1f, 1000.0f);
    m_camPosition = glm::vec3(0.0, 0.0, 2.0f);
//   // m_Scaling = glm::scale(glm::vec3(5.0,5.0,5.0));
//    m_View = glm::lookAt(
//            glm::vec3(0,0,-1),
//            glm::vec3(0,0,0),
//            glm::vec3(0,1,0));
//    //m_MVP = m_Projection * m_View; //* m_Scaling;
#endif

#ifdef SPHERE

    initializeScene();
    m_Projection = glm::perspective(90.0f, 1.5f, 0.1f, 1000.0f);
    m_Scaling = glm::scale(glm::vec3(5.0,5.0,5.0));

    m_camDirection = glm::vec3(0,0,5);
    m_camPosition = glm::vec3(0,0,0);
    m_View = glm::lookAt(
                            glm::vec3(0,0,0),
                            glm::vec3(0,0,0),
                            glm::vec3(0,1,0));
    m_MVP = m_Projection * m_View * m_Scaling;


#endif
    //initializeTexture();
    initializeCompressedTexture();
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

void RendererCS::resize(int w, int h){

    CHECK_GL(glViewport, 0,0,w,h);
    m_screenH = h;
    m_screenW = w;
    ALOGE("view port size: %d %d\n", w,h);

}

void RendererCS::loadComputeShader(const char *ComputeShader, GLuint &computeId)  {

    GLuint compShdrId = CHECK_GL(glCreateShader, GL_COMPUTE_SHADER);
    CHECK_GL(glShaderSource, compShdrId, 1, &ComputeShader, NULL);
    CHECK_GL(glCompileShader, compShdrId);

    int result, logLength;
    CHECK_GL(glGetShaderiv, compShdrId, GL_COMPILE_STATUS, &result);

    if(result != GL_TRUE){

        CHECK_GL(glGetShaderiv, compShdrId, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> ComputeShaderErrorMsg(logLength);
        CHECK_GL(glGetShaderInfoLog, compShdrId, logLength, NULL, &ComputeShaderErrorMsg[0]);
        ALOGE("Error while compiling compute shader--%s", &ComputeShaderErrorMsg[0]);
        exit(1);
    }

    computeId = CHECK_GL(glCreateProgram);
    CHECK_GL(glAttachShader, computeId, compShdrId);
    CHECK_GL(glLinkProgram, computeId);


    CHECK_GL(glGetProgramiv, computeId, GL_LINK_STATUS, &result);
    if(result != GL_TRUE){

        CHECK_GL(glGetProgramiv, computeId, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> ProgramErrMsg(logLength);
        glGetProgramInfoLog(computeId, logLength, NULL, &ProgramErrMsg[0]);
        ALOGE("Error while Linking porgram", &ProgramErrMsg[0]);
        exit(1);

    }

}

void RendererCS::initializeTexture()   {

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



    CHECK_GL(glGenTextures, 1, &m_TextureIdDXT);
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdDXT);
    CHECK_GL(glTexStorage2D, GL_TEXTURE_2D, 1, 0x83F1, vImageWidth, vImageHeight);
    CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);


    CHECK_GL(glGenBuffers, 2, m_PboId);
    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_PboId[0]);
    CHECK_GL(glBufferData, GL_PIXEL_UNPACK_BUFFER, (vImageHeight*vImageWidth)/2, NULL, GL_DYNAMIC_DRAW);
    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_PboId[1]);
    CHECK_GL(glBufferData, GL_PIXEL_UNPACK_BUFFER, (vImageHeight*vImageWidth)/2, NULL, GL_DYNAMIC_DRAW);
    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
}

void RendererCS::initializeCompressedTexture()   {



    CHECK_GL(glGenTextures, 1, &m_TextureIdDXT);
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdDXT);

#ifdef DXT1
    CHECK_GL(glTexStorage2D, GL_TEXTURE_2D, 1, 0x83F1, vImageHeight, vImageWidth );
#endif

#ifdef ASTC4x4
    CHECK_GL(glGenTextures, 1,&m_TextureIdCmp);
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdCmp);
    CHECK_GL(glTexStorage2D, GL_TEXTURE_2D, 1, COMPRESSED_RGBA_ASTC_4x4, vImageHeight, vImageWidth);
#endif


    CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);



}

void RendererCS::loadTextureDataDXT(int img_num)  {

    unsigned char *blocks;
    blocks = (unsigned char *)malloc( sizeof(unsigned char) * (vImageHeight *vImageWidth)/2);
    if(blocks == NULL)
        ALOGE("Block is NULL\n");
    char img_path[256];

    sprintf(img_path, "%s/DXT/Pixar_bmp/Pixar_bmp%05d.DXT1",m_TexturePath,img_num);
    ALOGE("Path %s", img_path);

    FILE *fp = fopen(img_path, "rb");



    std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
    fread(blocks, 1, (vImageHeight * vImageWidth)/2, fp);
    std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

    std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
    m_CPULoad.push_back(CPULoad_Time.count());



//    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_PboId[0]);
//
//    blocks = (GLubyte*)CHECK_GL(glMapBufferRange, GL_PIXEL_UNPACK_BUFFER, 0, (vImageHeight * vImageWidth)/2, GL_WRITE_ONLY);
//    if(blocks){
//
//        input.read(blocks, (vImageHeight * vImageWidth)/2);
//        CHECK_GL(glUnmapBuffer, GL_PIXEL_UNPACK_BUFFER);
//    }

    // GPU Loading........
    std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdDXT);
    CHECK_GL(glCompressedTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
             0,                // level (0 being the top level i.e. full size)
             0, 0,             // Offset
             vImageWidth,       // Width of the texture
             vImageHeight,      // Height of the texture,
             0x83F1,          // Data format
             vImageWidth*vImageHeight / 2, // Type of texture data
             blocks);

    std::chrono::high_resolution_clock ::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
    m_GPULoad.push_back(GPULoad_Time.count());

   // CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
   // ALOGE("fooooddd here\n");
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);
    free(blocks);
    fclose(fp);
}

static crn_uint8 *read_file_into_buffer(const char *pFilename, crn_uint32 &size)   {
    size = 0;

    FILE* pFile = NULL;
    pFile = fopen(pFilename, "rb");
    if (!pFile)
        return NULL;

    fseek(pFile, 0, SEEK_END);
    size = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    crn_uint8 *pSrc_file_data = static_cast<crn_uint8*>(malloc(std::max(1U, size)));
    if ((!pSrc_file_data) || (fread(pSrc_file_data, size, 1, pFile) != 1))
    {
        fclose(pFile);
        free(pSrc_file_data);
        size = 0;
        return NULL;
    }

    fclose(pFile);
    return pSrc_file_data;
}

void RendererCS::loadTextureDataCRN(int img_num)  {

    char img_path[256];
    sprintf(img_path, "%s/CRN/Pixar_bmp/Pixar_bmp%05d.CRN",m_TexturePath,img_num);
    ALOGE("Path%s", img_path);

    crn_uint32 src_file_size;

    // CPU Loading.....
    std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
    crn_uint8 *pSrc_file_data = read_file_into_buffer(img_path, src_file_size);
    std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

    std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
    m_CPULoad.push_back(CPULoad_Time.count());


    ALOGE("here\n");
    if (pSrc_file_data == NULL)
      ALOGE("error in reading file\n");


    std::chrono::high_resolution_clock::time_point CPUDecode_Start = std::chrono::high_resolution_clock::now();
    crnd::crn_texture_info tex_info;
    if (!crnd::crnd_get_texture_info(pSrc_file_data, src_file_size, &tex_info))
    {
        free(pSrc_file_data);
        ALOGE("crnd_get_texture_info() failed!\n");
    }

    ALOGE("here3..\n");
    const crn_uint32 width = std::max(1U, tex_info.m_width >> 0);
    const crn_uint32 height = std::max(1U, tex_info.m_height >> 0);
    const crn_uint32 blocks_x = std::max(1U, (width + 3) >> 2);
    const crn_uint32 blocks_y = std::max(1U, (height + 3) >> 2);
    const crn_uint32 row_pitch = blocks_x * crnd::crnd_get_bytes_per_dxt_block(tex_info.m_format);
    const crn_uint32 total_face_size = row_pitch * blocks_y;

    ALOGE("here3..\n");
    crnd::crnd_unpack_context pContext = crnd::crnd_unpack_begin(pSrc_file_data, src_file_size);

    std::chrono::high_resolution_clock::time_point CPUDecode_End = std::chrono::high_resolution_clock::now();

    std::chrono::nanoseconds CPUDecode_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_End - CPUDecode_Start);
    m_CPUDecode.push_back(CPUDecode_Time.count());


    ALOGE("here2...\n");
    //GPU Loading.......
    void *TextureData;
    TextureData = malloc(vImageWidth*vImageHeight / 2);

    std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
    crnd::crnd_unpack_level(pContext, &TextureData, total_face_size, row_pitch,0);
    if(TextureData == NULL) ALOGE("Texture Data is Null\n");
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdDXT);
    CHECK_GL(glCompressedTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
                              0,                // level (0 being the top level i.e. full size)
                              0, 0,             // Offset
                              vImageWidth,       // Width of the texture
                              vImageHeight,      // Height of the texture,
                              0x83F1,          // Data format
                              vImageWidth*vImageHeight / 2, // Type of texture data
                              TextureData);

    std::chrono::high_resolution_clock ::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
    m_GPULoad.push_back(GPULoad_Time.count());


    CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);

    free(TextureData);

}


void RendererCS::loadTextureDataASTC4x4(int img_num) {

    unsigned char *blocks;
    size_t  TexSz = vImageHeight * vImageWidth;
    blocks = (unsigned char *)malloc( sizeof(unsigned char) * TexSz);

    char img_path[256];

    sprintf(img_path, "%s/360MegaCoaster2k/ASTC4x4/360MegaCoaster2k%06d.astc",m_TexturePath,7);
    ALOGE("Path %s", img_path);
    unsigned char t[16];
    FILE *fp = fopen(img_path, "rb");
    fread(t,1,16,fp);
    std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
    size_t numBytes = fread(blocks, 1, TexSz, fp);
    if( numBytes <= 0 )
        ALOGE("Num bytes :%lld\n",numBytes);
    else
        ALOGE("Num bytes :%lld\n",numBytes);
    std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

    std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
    m_CPULoad.push_back(CPULoad_Time.count());



//    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_PboId[0]);
//
//    blocks = (GLubyte*)CHECK_GL(glMapBufferRange, GL_PIXEL_UNPACK_BUFFER, 0, (vImageHeight * vImageWidth)/2, GL_WRITE_ONLY);
//    if(blocks){
//
//        input.read(blocks, (vImageHeight * vImageWidth)/2);
//        CHECK_GL(glUnmapBuffer, GL_PIXEL_UNPACK_BUFFER);
//    }

    // GPU Loading........
    std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdCmp);
    CHECK_GL(glCompressedTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
             0,                // level (0 being the top level i.e. full size)
             0, 0,             // Offset
             vImageWidth,       // Width of the texture
             vImageHeight,      // Height of the texture,
             COMPRESSED_RGBA_ASTC_4x4,          // Data format
             TexSz, // Type of texture data
             blocks);

    std::chrono::high_resolution_clock ::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
    m_GPULoad.push_back(GPULoad_Time.count());

    // CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
    // ALOGE("fooooddd here\n");
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);
    free(blocks);
    fclose(fp);

}
void RendererCS::loadTextureDataASTC8x8(int img_num) {

    unsigned char *blocks;
    size_t  TexSz = vImageHeight * vImageWidth/4;
    blocks = (unsigned char *)malloc( sizeof(unsigned char) * TexSz);

    char img_path[256];

    sprintf(img_path, "%s/360MegaCoaster2k/ASTC8x8/360MegaCoaster2k%06d.astc",m_TexturePath,img_num);
    ALOGE("Path %s", img_path);

    FILE *fp = fopen(img_path, "rb");

    std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();

    if(fread(blocks, 1, TexSz, fp) <= 0)
        ALOGE("Block is NULL\n");
    std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

    std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
    m_CPULoad.push_back(CPULoad_Time.count());



//    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_PboId[0]);
//
//    blocks = (GLubyte*)CHECK_GL(glMapBufferRange, GL_PIXEL_UNPACK_BUFFER, 0, (vImageHeight * vImageWidth)/2, GL_WRITE_ONLY);
//    if(blocks){
//
//        input.read(blocks, (vImageHeight * vImageWidth)/2);
//        CHECK_GL(glUnmapBuffer, GL_PIXEL_UNPACK_BUFFER);
//    }

    // GPU Loading........
    std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdCmp);
    CHECK_GL(glCompressedTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
             0,                // level (0 being the top level i.e. full size)
             0, 0,             // Offset
             vImageWidth,       // Width of the texture
             vImageHeight,      // Height of the texture,
             COMPRESSED_RGBA_ASTC_8x8,          // Data format
             TexSz, // Type of texture data
             blocks);

    std::chrono::high_resolution_clock ::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
    m_GPULoad.push_back(GPULoad_Time.count());

    // CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
    // ALOGE("fooooddd here\n");
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);
    free(blocks);
    fclose(fp);

}
void RendererCS::loadTextureDataJPG(int img_num)   {

    char img_path[256];
    sprintf(img_path, "%s/360Shark%04d.jpg",m_TexturePath,img_num);
    ALOGE("Path%s", img_path);
    int w,h,n;

    unsigned char *ImageDataPtr = (unsigned char *)malloc((vImageHeight * vImageWidth * 4));
    FILE *fp = fopen(img_path, "rb");


    //CPU LOADING.....
    std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
    fread(ImageDataPtr, 1, vImageHeight * vImageWidth *4, fp);
    std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

    std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
    m_CPULoad.push_back(CPULoad_Time.count());


    // CPU Decoding.....
    std::chrono::high_resolution_clock::time_point CPUDecode_Start = std::chrono::high_resolution_clock::now();
    unsigned char *TextureData = stbi_load_from_memory(ImageDataPtr, (vImageHeight * vImageWidth * 4), &w, &h, &n, 4);
    std::chrono::high_resolution_clock::time_point CPUDecode_End = std::chrono::high_resolution_clock::now();

    std::chrono::nanoseconds CPUDecode_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_End - CPUDecode_Start);
    m_CPUDecode.push_back(CPUDecode_Time.count());
    free(ImageDataPtr);

    ALOGE("checking the loaded texture: %d %d %d \n", w, h, img_num);


    // GPU Loading........
    std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureId2);

    CHECK_GL(glTexSubImage2D, GL_TEXTURE_2D,
             0,
             0,0,
             vImageWidth,
             vImageHeight,
             GL_RGBA,
             GL_UNSIGNED_BYTE,
             TextureData);
    std::chrono::high_resolution_clock ::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
    m_GPULoad.push_back(GPULoad_Time.count());


    CHECK_GL(glBindTexture,GL_TEXTURE_2D, 0);

    stbi_image_free(TextureData);
    fclose(fp);

}

void RendererCS::draw(float AngleX, float AngleY)  {


  //CHECK_GL(glUseProgram,m_ComputeId);
  // Let's do some error checking
  int result;
//


  //CHECK_GL(glGetProgramiv,m_ComputeId, GL_ACTIVE_UNIFORMS, &result);
  //ALOGE("count of active uniforms%d",result);

// buffer for name
    GLchar * name = new GLchar[200];
// buffer for length
    GLsizei length = 100;
// Type of variable
    GLenum type;
// Count of variables
    GLint size;
// loop over active uniforms getting each's name
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

  //CHECK_GL(glGetActiveUniform, m_ComputeId, 0,200,&length,&size,&type, name);
  //ALOGE("index location%d\n", texLocInCS);
 // ALOGE("index location 2 %d\n", texLocOutCS);

  m_TextureNumber = m_TextureNumber % 50+ 1;
  //loadTextureDataJPG(m_TextureNumber);
  //loadTextureDataDXT(m_TextureNumber);
  //loadTextureDataCRN(m_TextureNumber);
  loadTextureDataASTC4x4(m_TextureNumber);

  //std::chrono::high_resolution_clock::time_point compute_start = std::chrono::high_resolution_clock::now();
  //CHECK_GL(glBindBufferBase,GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
  //CHECK_GL(glBufferData, GL_SHADER_STORAGE_BUFFER, sizeof(scale), scale, GL_DYNAMIC_COPY);
 // CHECK_GL(glDispatchCompute, vImageWidth/8, vImageHeight/8, 1);
  //CHECK_GL(glMemoryBarrier, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  //std::chrono::high_resolution_clock::time_point compute_end = std::chrono::high_resolution_clock::now();
 // std::chrono::nanoseconds compute_time = std::chrono::duration_cast<std::chrono::nanoseconds>(compute_end - compute_start);
 // ALOGE("Compute Time %lld\n", compute_time.count());


    //m_camPosition.z += 0.01;
//    if(m_AngleX != AngleX || m_AngleY != AngleY) {
//        m_AngleX = AngleX;
//        m_AngleY = AngleY;
//        glm::vec4 tempCam =
//                glm::rotate(AngleX, glm::vec3(1, 0, 0)) * glm::rotate(AngleY, glm::vec3(0, 1, 0)) *
//                glm::vec4(m_camDirection, 1);
//        tempCam = tempCam / tempCam.w;
//        m_camDirection = glm::vec3(tempCam);
//    }
    m_View = glm::lookAt(
            m_camPosition,
            m_camDirection,
            glm::vec3(0,1,0));

    m_MVP = m_Projection * m_View * m_Scaling;


  CHECK_GL(glUseProgram,m_ProgramId);
  CHECK_GL(glClear,GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  CHECK_GL(glUniformMatrix4fv, matrixID, 1, GL_FALSE, &m_MVP[0][0]);

  CHECK_GL(glActiveTexture, GL_TEXTURE0);


  CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdCmp);

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
  ALOGE("Num indices %d\n", m_NumIndices);
  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
  std::chrono::nanoseconds frame_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  m_TotalFps.push_back(frame_time.count());

  CHECK_GL(glDisableVertexAttribArray,posLoc);
  CHECK_GL(glDisableVertexAttribArray,uvLoc);
  CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, 0);
  CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);
  CHECK_GL(glUseProgram, 0);

  m_numframes = m_numframes + 1;
  if(m_numframes>=10){

      ull CPU_load, CPU_decode, GPU_Load, FPS;
      if(!m_CPULoad.empty())
          CPU_load = std::accumulate(m_CPULoad.begin(), m_CPULoad.end(), 0)/m_CPULoad.size();

      if(!m_CPUDecode.empty())
          CPU_decode = std::accumulate(m_CPUDecode.begin(), m_CPUDecode.end(), 0)/m_CPUDecode.size();

      if(!m_GPULoad.empty())
          GPU_Load = std::accumulate(m_GPULoad.begin(), m_GPULoad.end(), 0)/m_GPULoad.size();

      if(!m_TotalFps.empty())
          FPS = std::accumulate(m_TotalFps.begin(), m_TotalFps.end(), 0)/m_TotalFps.size();
      ALOGV("CPU Load Time : %lld--%d\n", CPU_load, m_CPULoad.size());
      ALOGV("CPU Decode Time: %lld--%d\n", CPU_decode, m_CPUDecode.size());
      ALOGV("GPU Load Time: %lld--%d\n", GPU_Load, m_GPULoad.size());

      ALOGV("FPS: %lld--%d\n", FPS, m_TotalFps.size());
      m_CPULoad.clear();
      m_CPUDecode.clear();
      m_GPULoad.clear();
      m_TotalFps.clear();


      m_numframes = 0;
  }


}

void RendererCS::drawDXT(float AngleX, float AngleY)   {

    CHECK_GL(glUseProgram,m_ComputeId2);
    // Let's do some error checking
    int result;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    CHECK_GL(glGetProgramiv,m_ComputeId2, GL_ACTIVE_UNIFORMS, &result);
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


    //CHECK_GL(glGetActiveUniform, m_ComputeId, 0,200,&length,&size,&type, name);
    ALOGE("index location%d\n", texLocInCS);
    ALOGE("index location 2 %d\n", texLocOutCS);

    //m_TextureNumber = m_TextureNumber % 20 + 1;
   // loadTextureDataJPG(m_TextureNumber);


    std::chrono::high_resolution_clock::time_point compute_start = std::chrono::high_resolution_clock::now();

    CHECK_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, m_ssbo);

    CHECK_GL(glBufferData, GL_SHADER_STORAGE_BUFFER, (vImageHeight * vImageWidth)/2, 0, GL_STATIC_COPY);
    CHECK_GL(glBindBufferBase,GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
    CHECK_GL(glDispatchCompute, vImageWidth/4, vImageHeight/4, 1);
    CHECK_GL(glMemoryBarrier, GL_ALL_BARRIER_BITS);
    std::chrono::high_resolution_clock::time_point compute_end = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds compute_time = std::chrono::duration_cast<std::chrono::nanoseconds>(compute_end - compute_start);
    ALOGE("Compute Time %lld\n", compute_time.count());
    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, m_ssbo);

//    CHECK_GL(glCopyBufferSubData, GL_COPY_READ_BUFFER, GL_PIXEL_UNPACK_BUFFER, 0, 0,(vImageWidth*vImageHeight)/2 );

    CHECK_GL(glActiveTexture, GL_TEXTURE0);


    CHECK_GL(glBindTexture, GL_TEXTURE_2D, m_TextureIdDXT);

    CHECK_GL(glCompressedTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
                              0,                // level (0 being the top level i.e. full size)
                              0, 0,             // Offset
                              vImageWidth,       // Width of the texture
                              vImageHeight,      // Height of the texture,
                              0x83F1,          // Data format
                              vImageWidth*vImageHeight / 2, // Type of texture data
                              0);     //

    CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
    m_camPosition.z += 0.01;
    m_View = glm::lookAt(
            m_camPosition,
            glm::vec3(0,0,0),
            glm::vec3(0,1,0));
    m_MVP = m_Projection * m_View;
    CHECK_GL(glUseProgram,m_ProgramId);
    CHECK_GL(glClear,GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    CHECK_GL(glUniformMatrix4fv, matrixID, 1, GL_FALSE, &m_MVP[0][0]);


    CHECK_GL(glEnableVertexAttribArray, posLoc);
    CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, m_VertexBuffer);
    CHECK_GL(glVertexAttribPointer,posLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, 0);

    CHECK_GL(glEnableVertexAttribArray,uvLoc);
    CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, m_UVBuffer);
    CHECK_GL(glVertexAttribPointer,uvLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, 0);
    ALOGE("Num Indices: %d\n", m_NumIndices);

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
JNIEXPORT void JNICALL Java_com_example_psrihariv_gentcmobile_GenTCJNILib_draw(JNIEnv* env, jobject obj, jfloat AngleX, jfloat AngleY);
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
Java_com_example_psrihariv_gentcmobile_GenTCJNILib_draw(JNIEnv* env, jobject obj, jfloat AngleX, jfloat AngleY) {
    if (g_renderer) {
        //g_renderer->drawDXT(angle);
        g_renderer->draw(AngleX, AngleY);
        ALOGE("error angle %f - %f\n",AngleX, AngleY);
    }
}
