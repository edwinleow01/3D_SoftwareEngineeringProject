#pragma once

#include "pch.h"
#include "system.h"
#include <unordered_map>
#include <string>
#include <ft2build.h>
#include <glew.h>
#include <glm.hpp>
#include <map>
#include "FontShader.h"
#include <fstream>
#include <sstream>
#include <gtc/matrix_transform.hpp>
#include FT_FREETYPE_H

namespace Framework {

    struct Character {
        GLuint TextureID;   // Glyph texture ID
        glm::ivec2 Size;    // Glyph size
        glm::ivec2 Bearing; // Offset from baseline
        GLuint Advance;     // Horizontal advance
    };

    class FontSystem : public ISystem {
    public:
        FontSystem();
        ~FontSystem();
        void Initialize();
        void Update(float deltaTime) override;
        std::string GetName() override { return "FontSystem"; }

        // Font loading & selection
        bool LoadFont(const std::string& fontPath, int fontSize, const std::string& fontName);
        void SetActiveFont(const std::string& fontName);

        void RenderText2D(const std::string& text,
            float x, float y, float z,
            float scale,
            glm::vec3 color,
            float screenWidth,
            float screenHeight,
            int alignment = 0,
            float opacity = 1.0f);

        void RenderText3D(const std::string& text,
            const glm::mat4& model,
            const glm::mat4& view,
            const glm::mat4& projection,
            float scale,
            glm::vec3 color,
            int alignment,
            float opacity = 1.0f);

    private:
        FT_Library ft;
        std::unordered_map<std::string, std::unordered_map<char, Character>> FontCache;
        std::string activeFont;

        GLuint VAO{}, VBO{};
        FontShader shader;
        FontShader shader3D; // For 3D text
    };

    // Global FontSystem object
    extern FontSystem fontSystem;

} // namespace Framework
