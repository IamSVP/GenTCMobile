//
// Created by psrihariv on 3/1/2016.
//

#include "GenTCjni.h"


const char *vVertexProg =
        "#version 300 es\n"
                ""
                "attribute vec3 position;\n"
                "attribute vec2 texCoord;\n"
                ""
                "varying vec2 uv;\n"
                ""
                "void main() {\n"
                "  gl_Position = vec4(position, 1.0);\n"
                "  uv = texCoord;\n"
                "}\n";

const char *vFragProg =
        "#version 300 es\n"
                ""
                "varying vec2 uv;\n"
                ""
                "uniform sampler2D tex;\n"
                ""
                "void main() {\n"
                "  gl_FragColor = vec4(texture2D(tex, uv).rgb, 1);\n"
                "}\n";


void Renderer::loadShaders(const char *VertexShader, const char *FragmentShader){

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
    glGetProgramInfoLog(m_ProgramId, logLength, NULL, &ProgramErrMsg[0]);
    ALOGE("Error while Linking the program", &ProgramErrMsg[0]);
  }



}

// Look up more on gl_context and egl context and how's it handled
Renderer::Renderer():m_EglContext(eglGetCurrentContext())
{
}

void Renderer::init(const char * path){

    strcpy(m_TexturePath, path);

    // compile shaders
    loadShaders(vVertexProg, vFragProg);

    


}