#pragma once
#include "../Demo.hpp"
#include "GL_Helpers/GL_Shader.hpp"
#include "GL_Helpers/GL_Camera.hpp"

#include "GL_Helpers/GL_Mesh.hpp"
#include "GL_Helpers/GL_Camera.hpp"
#include "GL_Helpers/GL_Texture.hpp"



struct ImageProcessStack;

void Line(glm::ivec2 x0, glm::ivec2 x2, std::vector<glm::vec4> &image, int width);

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

struct LaplacianOfGaussian : public ImageProcess
{
    LaplacianOfGaussian(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;

    void RecalculateKernel();
    int size=9;
    float sigma=1.0f;
    int maxSize = 257;
    std::vector<float> kernel;
    GLuint kernelBuffer;

    bool shouldRecalculateKernel=true;
};

struct DifferenceOfGaussians : public ImageProcess
{
    DifferenceOfGaussians(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;

    void RecalculateKernel();
    int size=9;
    float sigma1=1.0f;
    float sigma2=3.0f;
    int maxSize = 257;
    std::vector<float> kernel;
    GLuint kernelBuffer;

    bool shouldRecalculateKernel=true;
};

struct CannyEdgeDetector : public ImageProcess
{
    CannyEdgeDetector(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;

    void RecalculateKernel();
    int size=3;
    float sigma=1;
    int maxSize = 257;
    float threshold=0.4f;
    std::vector<float> kernel;
    GLuint kernelBuffer;

    GLint gradientShader;
    GLint edgeShader;
    GLint thresholdShader;
    GLint hysteresisShader;
    GL_TextureFloat blurTexture;
    GL_TextureFloat gradientTexture;
    GL_TextureFloat edgeTexture;
    GL_TextureFloat thresholdTexture;
    
    int outputStep=4;

    bool shouldRecalculateKernel=true;
};

struct ArbitraryFilter : public ImageProcess
{
    ArbitraryFilter(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;

    void RecalculateKernel();
    int sizeX=3;
    int sizeY=3;
    int maxSize = 257;

    bool normalize=false;
    std::vector<float> kernel;
    std::vector<float> normalizedKernel;
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

struct EdgeLinking : public ImageProcess
{
    EdgeLinking(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);

    CannyEdgeDetector *cannyEdgeDetector;

    bool cannyChanged=true;

    int windowSize=4;

    bool doProcess=true;

    std::vector<glm::vec4> gradientData;
    std::vector<glm::vec4> edgeData;
    std::vector<glm::vec4> linkedEdgeData;

    float magnitudeThreshold= 0.4f;
    float angleThreshold = 0.4f;
};

struct Gradient : public ImageProcess
{
    Gradient(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;

    enum class RenderMode
    {
        Magnitude=0,
        GradientX=1,
        GradientY=2,
        GradientXY=3,
        angle=4
    } renderMode = RenderMode::Magnitude;

    float minMag = 0;
    float maxMag = 1;
    float angleRange = PI;
    float angle = 0;
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


struct FFTBlur : public ImageProcess
{
    FFTBlur(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);

    enum class Type 
    {
        BOX,
        CIRCLE,
        GAUSSIAN
    } filter = Type::GAUSSIAN;

    int radius=10;
    float sigma = 0.1f;

    GLuint fftTexture;
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