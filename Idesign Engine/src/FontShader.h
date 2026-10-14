///////////////////////////////////////////////////////////////////////////////
///
///	\file FontShader.h
/// 
///	* This class manages the GLSL shader program specifically designed for rendering
/// * text in Unnamed Engine
/// 
/// 
///	Authors: Joshua Sim Yue Chen
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <glew.h>
#include <string>
#include <iostream>
#include <glm.hpp>

class FontShader {
public:
    GLuint ID;

    /**
   * \brief Constructor for the FontShader class.
   * Initializes the shader ID to 0.
   */
    FontShader() : ID(0) {}

    /**
     * \brief Compiles and links the vertex and fragment shaders into a shader program.
     *
     * This function compiles the provided vertex and fragment shader source code,
     * attaches the shaders to the shader program, links them, and deletes the
     * individual shaders after linking.
     *
     * \param vertexSource The source code for the vertex shader.
     * \param fragmentSource The source code for the fragment shader.
     */
    void Compile(const char* vertexSource, const char* fragmentSource) {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        CheckCompileErrors(vertexShader, "VERTEX");

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        CheckCompileErrors(fragmentShader, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        CheckCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    /**
     * \brief Activates the shader program.
     *
     * This function sets the current OpenGL shader program to the one managed by this class.
     */
    void Use() {
        glUseProgram(ID);
    }

    /**
    * \brief Sets a 4x4 matrix uniform in the shader program.
    *
    * This function locates the uniform variable by name and assigns it the given
    * 4x4 matrix value.
    *
    * \param name The name of the uniform variable in the shader.
    * \param value The glm::mat4 value to assign to the uniform.
    */
    void SetUniform(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }

    /**
    * \brief Sets a 3D vector uniform in the shader program.
    *
    * This function locates the uniform variable by name and assigns it the given
    * 3D vector value.
    *
    * \param name The name of the uniform variable in the shader.
    * \param value The glm::vec3 value to assign to the uniform.
    */
    void SetUniform(const std::string& name, const glm::vec3& value) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), value.x, value.y, value.z);
    }

    /**
    * \brief Sets a float uniform in the shader program.
    * \param name The name of the uniform variable.
    * \param value The float value to assign.
    */
    void SetUniform(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

private:
    void CheckCompileErrors(GLuint shader, std::string type) {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "| ERROR::SHADER-COMPILATION: " << type << " |\n" << infoLog << "\n";
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "| ERROR::PROGRAM-LINKING: " << type << " |\n" << infoLog << "\n";
            }
        }
    }
};
