#pragma once
#include "../Demo.hpp"
#include "GL_Helpers/GL_Shader.hpp"
#include "GL_Helpers/GL_Camera.hpp"

#include "GL_Helpers/GL_Mesh.hpp"
#include "GL_Helpers/GL_Camera.hpp"
#include "GL_Helpers/GL_Texture.hpp"

struct ImageProcessStack;

struct ImageProcess
{
    ImageProcess(std::string name, std::string shaderFileName, bool enabled);
    virtual void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    virtual void SetUniforms();
    virtual void RenderGui();
    std::string shaderFileName;
    std::string name;
    GLint shader;

    ImageProcessStack *imageProcessStack;

    bool enabled=true;
};

struct ImageProcessStack
{
    ImageProcessStack();
    std::vector<ImageProcess*> imageProcesses;
    GLuint Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void AddProcess(ImageProcess* imageProcess);
    void RenderHistogram();

    void RenderGUI();

    bool histogramR=true, histogramG=true, histogramB=true, histogramGray=false;
    float minValueGray, maxValueGray;
    GLint histogramShader, resetHistogramShader, renderHistogramShader;
    GLuint histogramBuffer, boundsBuffer;
    GL_TextureFloat histogramTexture;
};


struct ColorContrastStretch : public ImageProcess
{
    ColorContrastStretch(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    glm::vec3 lowerBound = glm::vec3(0);
    glm::vec3 upperBound = glm::vec3(1);
    bool global = false;
    float globalLowerBound=0;
    float globalUpperBound=1;
};

struct GrayScaleContrastStretch : public ImageProcess
{
    GrayScaleContrastStretch(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    float lowerBound = 0;
    float upperBound = 1;
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
    Equalize(bool enabled=true);
    void SetUniforms() override;
    void RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;
    
    bool color=true;
    
    GLuint lutBuffer;
    GLuint pdfBuffer;
    GLint histogramBuffer, boundsBuffer;

    GLint resetHistogramShader;
    GLint histogramShader;
    GLint buildPdfShader;
    GLint buildLutShader;
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