#pragma once

#include <GL/glew.h>

#include <stb_image.h>
#include <iostream>
#include <glm/vec4.hpp>

struct TextureCreateInfo {
    GLint wrapS = GL_MIRRORED_REPEAT;
    GLint wrapT = GL_MIRRORED_REPEAT;
    GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;
    GLint magFilter = GL_LINEAR;
    bool generateMipmaps=true;
    bool srgb=false;
    bool flip=false;
};

class GL_TextureFloat {
public:
    GL_TextureFloat() : loaded(false), width(0), height(0), nChannels(0){}
    
    GL_TextureFloat(std::string filename, TextureCreateInfo createInfo) {
        this->filename = filename;
        
        glGenTextures(1, &glTex);
        glBindTexture(GL_TEXTURE_2D, glTex);

        if(createInfo.flip) stbi_set_flip_vertically_on_load(true);  
        // data = stbi_loadf(filename.c_str(), &width, &height, &nChannels, 0);
        
        uint8_t* charData = stbi_load(filename.c_str(), &width, &height, &nChannels, 4);
        if(charData)
        {
            data = (glm::vec4*)malloc(width * height * sizeof(glm::vec4));
            for(int i=0; i<width * height; i++)
            {
                data[i].r = (float)charData[i * 4 + 0] / 255.0f;
                data[i].g = (float)charData[i * 4 + 1] / 255.0f;
                data[i].b = (float)charData[i * 4 + 2] / 255.0f;
                data[i].a = (float)charData[i * 4 + 3] / 255.0f;
            }
        }

#if 0
        for(int y=0; y<height; y++)
        {
            for(int x=0; x<height; x++)
            {
                int inx = y * width + x;
                std::cout << data[inx].x << " ";
            }
            std::cout << std::endl;
        }
#endif

        stbi_image_free(charData);

        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, data);
        }
        else
        {
            std::cout << "Texture:Constructor: ERROR::Failed to load texture" << std::endl;
            return;
        } 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, createInfo.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, createInfo.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, createInfo.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, createInfo.magFilter);
        if(createInfo.generateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    

        glBindTexture(GL_TEXTURE_2D, 0);
        loaded = true;        
    }
    
    GL_TextureFloat(int width, int height, TextureCreateInfo createInfo) : width(width), height(height) {
        glGenTextures(1, &glTex);
        glBindTexture(GL_TEXTURE_2D, glTex);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, createInfo.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, createInfo.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, createInfo.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, createInfo.magFilter);
    
        glBindTexture(GL_TEXTURE_2D, 0);
        loaded = true;  
        data = nullptr;
    }

    void Unload()
    {
        glDeleteTextures(1, &glTex);
        if(data != nullptr) free(data);
    }

    glm::vec4 *Data()
    {
        return data;
    }

    GLuint glTex;
    bool loaded=false;
    int width, height, nChannels;
    std::string filename;
    glm::vec4 *data= nullptr;

};

class GL_Texture {
public:
    GL_Texture() : loaded(false), width(0), height(0), nChannels(0){}
    
        
    GL_Texture(std::string filename, TextureCreateInfo createInfo) {
        this->filename = filename;
        
        glGenTextures(1, &glTex);
        glBindTexture(GL_TEXTURE_2D, glTex);

        
        if(createInfo.flip) stbi_set_flip_vertically_on_load(true);  
        // data = stbi_loadf(filename.c_str(), &width, &height, &nChannels, 0);
        data = stbi_load(filename.c_str(), &width, &height, &nChannels, 4);
        if(createInfo.flip) stbi_set_flip_vertically_on_load(false);  
        
        
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            std::cout << "Texture:Constructor: ERROR::Failed to load texture" << std::endl;
            return;
        } 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, createInfo.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, createInfo.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, createInfo.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, createInfo.magFilter);
        if(createInfo.generateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);

        

        glBindTexture(GL_TEXTURE_2D, 0);
        loaded = true;        
    }

    GL_Texture(int width, int height, TextureCreateInfo createInfo) : width(width), height(height) {
        glGenTextures(1, &glTex);
        glBindTexture(GL_TEXTURE_2D, glTex);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, createInfo.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, createInfo.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, createInfo.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, createInfo.magFilter);
    
        glBindTexture(GL_TEXTURE_2D, 0);
        loaded = true;        
    }

    void Unload()
    {
        glDeleteTextures(1, &glTex);
        stbi_image_free(data);
    }

    unsigned char *Data()
    {
        return data;
    }

    GLuint glTex;
    bool loaded=false;
    int width, height, nChannels;
    std::string filename;
    unsigned char *data;

};

class GL_Texture3D {
public:
    GLuint glTex;
    int size, nChannels;
};
