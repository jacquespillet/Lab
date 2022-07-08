#pragma once

#include <GL/glew.h>
#include <stb_image.h>

struct TextureCreateInfo {
    GLint wrapS = GL_MIRRORED_REPEAT;
    GLint wrapT = GL_MIRRORED_REPEAT;
    GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;
    GLint magFilter = GL_LINEAR;
    bool generateMipmaps=true;
    bool srgb=false;
    bool flip=true;
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
        
        uint8_t* charData = stbi_load(filename.c_str(), &width, &height, &nChannels, 0);
        data = (float*)malloc(width * height * nChannels * sizeof(float));
        for(int i=0; i<width * height * nChannels; i++)
        {
            data[i] = (float)charData[i] / 255.0f;
        }
        stbi_image_free(charData);

        std::cout << "Texture:Constructor: Num channels " << filename << "  " <<  nChannels << std::endl;
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
    }

    void Unload()
    {
        glDeleteTextures(1, &glTex);
        stbi_image_free(data);
    }

    float *Data()
    {
        return data;
    }

    GLuint glTex;
    bool loaded=false;
    int width, height, nChannels;
    std::string filename;
    float *data;

};

class GL_Texture {
public:
    GL_Texture() : loaded(false), width(0), height(0), nChannels(0){}
    
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

    float *Data()
    {
        return data;
    }

    GLuint glTex;
    bool loaded=false;
    int width, height, nChannels;
    std::string filename;
    float *data;

};

class GL_Texture3D {
public:
    GLuint glTex;
    int size, nChannels;
};
