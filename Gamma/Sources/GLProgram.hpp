#pragma once

#include "utils.hpp"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GL_SHADER_SOURCE(CODE) #CODE

using std::string;

class GLProgram
{
public:
	GLProgram		(const string& vertexSource, const string& fragmentSource);
	GLProgram		(const string& vertexSource, const string& geometrySource, const string& fragmentSource);

	~GLProgram(void);

	GLuint          getHandle(void) const { return m_glProgram; }
	GLint           getAttribLoc(const string& name) const;
	GLint           getUniformLoc(const string& name) const;

	void            use(void);
    void            addVAOs(GLuint *arr, int num);
    void            bindVAO(int ind);

	static GLuint   createGLShader(GLenum type, const string& typeStr, const string& source);
	static void     linkGLProgram(GLuint prog);

	// Static collection of all compiled programs
	static GLProgram* get(const string &name);
	static void		  set(const string &name, GLProgram *prog);
    static void       clearCache();

    void             setUniform(int loc, unsigned int v) { if (loc >= 0) glUniform1ui(loc, v); }
    void             setUniform(int loc, int v) { if (loc >= 0) glUniform1i(loc, v); }
	void             setUniform(int loc, float v) { if (loc >= 0) glUniform1f(loc, v); }
	void             setUniform(int loc, const glm::vec2& v) { if (loc >= 0) glUniform2f(loc, v.x, v.y); }
	void             setUniform(int loc, const glm::vec3& v) { if (loc >= 0) glUniform3f(loc, v.x, v.y, v.z); }
    void             setUniform(int loc, const glm::vec4& v) { if (loc >= 0) glUniform4f(loc, v.x, v.y, v.z, v.w); }
	void             setUniform(int loc, const glm::mat4& m) { if (loc >= 0) glUniformMatrix4fv(loc, 1, false, glm::value_ptr(m)); }

    template <typename T>
    void setUniform(const string& name, T v) { 
        GLint loc = getUniformLoc(name);
        if (loc == -1) std::cout << "Uniform name error: " << name << std::endl;
        setUniform(loc, v);
    }

	void			 setAttrib(int loc, int size, GLenum type, int stride, GLuint buffer, const void* pointer);
	void             setAttrib(int loc, int size, GLenum type, int stride, const void* pointer) { setAttrib(loc, size, type, stride, (GLuint)NULL, pointer); }
	void			 resetAttribs(void);

private:
	void            init(const string& vertexSource, const string& geometrySource, const string& fragmentSource);

private:
	GLProgram(const GLProgram&) = delete;
	GLProgram& operator=(const GLProgram&) = delete;

private:
	// Map that contains all compiled GLPrograms
	static std::map<string, GLProgram*> s_programs;
	
    std::vector<GLuint> vaos;
	int				m_numAttribs = 0;
	GLuint          m_glVertexShader;
	GLuint          m_glGeometryShader;
	GLuint          m_glFragmentShader;
	GLuint          m_glProgram;
};