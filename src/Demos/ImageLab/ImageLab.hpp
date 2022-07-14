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
    virtual bool RenderGui();
    std::string shaderFileName;
    std::string name;
    GLint shader;

    ImageProcessStack *imageProcessStack;

    bool enabled=true;
    bool CheckChanges();
};

struct ImageProcessStack
{
    GL_TextureFloat tex0;
    GL_TextureFloat tex1;

    ImageProcessStack();
    void Resize(int width, int height);
    std::vector<ImageProcess*> imageProcesses;
    GLuint Process(GLuint textureIn, int width, int height);
    void AddProcess(ImageProcess* imageProcess);
    void RenderHistogram();

    bool RenderGUI();

    bool histogramR=true, histogramG=true, histogramB=true, histogramGray=false;
    float minValueGray, maxValueGray;
    GLint histogramShader, resetHistogramShader, renderHistogramShader;
    GLuint histogramBuffer, boundsBuffer;
    GL_TextureFloat histogramTexture;

    bool changed=false;
};

//FFT :
// https://www.fftw.org/fftw3_doc/Multi_002dDimensional-DFTs-of-Real-Data.html
// https://www.fftw.org/fftw3_doc/Multi_002dDimensional-DFTs-of-Real-Data.html



struct ColorContrastStretch : public ImageProcess
{
    ColorContrastStretch(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
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
    bool RenderGui() override;
    float lowerBound = 0;
    float upperBound = 1;
};

struct Negative : public ImageProcess
{
    Negative(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
};

struct Threshold : public ImageProcess
{
    Threshold(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
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
    bool RenderGui() override;
    int numLevels=255;
};

struct Transform : public ImageProcess
{
    Transform(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    glm::vec2 translation = glm::vec2(0);
    glm::vec2 scale = glm::vec2(1);
    glm::vec2 shear = glm::vec2(0);
    float theta=0;
    int interpolation=0;

    bool flipX=false;
    bool flipY=false;
};

struct Resampling : public ImageProcess
{
    Resampling(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    int pixelSize=1;
};

struct AddNoise : public ImageProcess
{
    AddNoise(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    float density = 0;
    float intensity = 1;
    bool randomColor=false;
};

struct SmoothingFilter : public ImageProcess
{
    SmoothingFilter(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    int size=3;
};

struct SharpenFilter : public ImageProcess
{
    SharpenFilter(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
};

struct SobelFilter : public ImageProcess
{
    SobelFilter(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    bool vertical=true;
};

struct MedianFilter : public ImageProcess
{
    MedianFilter(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    bool vertical=true;
};

struct MinMaxFilter : public ImageProcess
{
    MinMaxFilter(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    int size = 3;
    bool doMin=true;
};

struct GaussianBlur : public ImageProcess
{
    GaussianBlur(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;

    void RecalculateKernel();
    int size=3;
    float sigma=1;
    int maxSize = 257;
    std::vector<float> kernel;
    GLuint kernelBuffer;

    bool shouldRecalculateKernel=true;
};

struct GammaCorrection : public ImageProcess
{
    GammaCorrection(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    float gamma=2.2f;
};

struct Equalize : public ImageProcess
{
    Equalize(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
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


struct FFT : public ImageProcess
{
    FFT(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
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

    bool shouldProcess=true;
    GLuint outTexture;

    bool firstFrame=true;

};