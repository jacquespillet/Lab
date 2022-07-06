#pragma once
#include "../Demo.hpp"
#include "GL_Helpers/GL_Shader.hpp"
#include "GL_Helpers/GL_Camera.hpp"

#include "GL_Helpers/GL_Mesh.hpp"
#include "GL_Helpers/GL_Camera.hpp"
#include "GL_Helpers/GL_Texture.hpp"

struct ImageProcess
{
    ImageProcess(std::string name, std::string shaderFileName, bool enabled);
    virtual void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    virtual void SetUniforms();
    virtual void RenderGui();
    std::string shaderFileName;
    std::string name;
    GLint shader;

    bool enabled=true;
};

struct ImageProcessStack
{
    std::vector<ImageProcess*> imageProcesses;
    GLuint Process(GLuint textureIn, GLuint textureOut, int width, int height);
};


struct ColorContrastStretch : public ImageProcess
{
    ColorContrastStretch(GL_TextureFloat *texture, bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    glm::vec3 lowerBound = glm::vec3(0);
    glm::vec3 upperBound = glm::vec3(1);
    GL_TextureFloat *texture;
    
    bool global = false;
    std::vector<float> histogramR;
    std::vector<float> histogramG;
    std::vector<float> histogramB;
    float globalLowerBound=0;
    float globalUpperBound=1;
};

struct ContrastStrech : public ImageProcess
{
    ContrastStrech(GL_TextureFloat *texture, bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    float lowerBound = 0;
    float upperBound = 1;
    std::vector<float> histogramGray;
    GL_TextureFloat *texture;
    
};

struct Negative : public ImageProcess
{
    Negative(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
};

struct Threshold : public ImageProcess
{
    Threshold(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    bool global=false;
    float globalLower = 0;
    float globalUpper = 1;
    glm::vec3 lower = glm::vec3(0);
    glm::vec3 upper = glm::vec3(1);
};

struct Quantization : public ImageProcess
{
    Quantization(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    int numLevels=255;
};

struct Resampling : public ImageProcess
{
    Resampling(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    int pixelSize=1;
};

struct GammaCorrection : public ImageProcess
{
    GammaCorrection(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    float gamma=2.2f;
};

struct Equalize : public ImageProcess
{
    Equalize(GL_TextureFloat *texture, bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    GL_TextureFloat *texture;
    std::vector<float> histogramR;
    std::vector<float> histogramG;
    std::vector<float> histogramB;
};



class ImageLab : public Demo {
public : 
    ImageLab();
    void Load();
    void Render();
    void RenderGUI();
    void Unload();

    void MouseMove(float x, float y);
    void LeftClickDown();
    void LeftClickUp();
    void RightClickDown();
    void RightClickUp();
    void Scroll(float offset);

    void Process();

private:
    clock_t t;
    float deltaTime;
    float elapsedTime;

    GL_Shader MeshShader;

    GL_Mesh* Quad;
    GL_TextureFloat texture;
    GL_TextureFloat tmpTexture;
    

    ImageProcessStack imageProcessStack;


};