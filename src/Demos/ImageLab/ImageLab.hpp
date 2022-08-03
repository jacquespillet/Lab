#pragma once
#include "../Demo.hpp"
#include "GL_Helpers/GL_Shader.hpp"
#include "GL_Helpers/GL_Camera.hpp"

#include "GL_Helpers/GL_Mesh.hpp"
#include "GL_Helpers/GL_Camera.hpp"
#include "GL_Helpers/GL_Texture.hpp"
#include <complex>

struct ImDrawList;

struct ImageProcessStack;


void DrawLine(glm::ivec2 x0, glm::ivec2 x2, std::vector<glm::vec4> &image, int width, int height, glm::vec4 color);

struct Curve
{
    bool Curve::Render(ImDrawList* drawList, uint32_t curveColor);
    struct bezierPoint
    {
        glm::vec2 point;
        glm::vec2 control;
    };
    std::vector<bezierPoint> controlPoints;
    std::vector<glm::vec2> path;
    std::vector<float> data;

    glm::vec2 GraphToScreen(glm::vec2 coord);
    glm::vec2 ScreenToGraph(glm::vec2 coord);

    float pointRadius=8;
    
    int numEval = 256;
    
    glm::vec2 origin;
    glm::vec2 size = glm::vec2(128);
        
    float Evaluate(float t);
    void BuildPath();
    struct
    {
        bool movingPoint=false;
        bool movingControlPoint=false;
        int index=0;
        glm::vec2 diff;
    } mouseState;
};


struct ImageProcess
{
    ImageProcess(std::string name, std::string shaderFileName, bool enabled);
    virtual void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    virtual void SetUniforms();
    virtual bool RenderGui();
    virtual bool RenderOutputGui();
    virtual void Unload(){
        glDeleteProgram(shader);
    }

    virtual bool MouseMove(float x, float y) {return false;}
    virtual bool MousePressed() {return false;}
    virtual bool MouseReleased() {return false;}
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
    GLuint Process();
    void AddProcess(ImageProcess* imageProcess);
    void RenderHistogram();
    void Unload();

    bool RenderGUI();

    bool histogramR=true, histogramG=true, histogramB=true, histogramGray=false;
    float minValueGray, maxValueGray;
    GLint histogramShader, resetHistogramShader, renderHistogramShader, clearTextureShader;
    GLuint histogramBuffer, boundsBuffer;
    GL_TextureFloat histogramTexture;

    glm::vec2 outputGuiStart;

    std::vector<glm::vec4> outputImage;

    bool changed=false;

    // int width = 1024;
    // int height = 900;
    
    float zoomLevel=1;
    int width = 512;
    int height = 512;
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

struct LocalThreshold : public ImageProcess
{
    LocalThreshold(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;

    int size=3;
    float a=1;
    float b=1;
};

struct Threshold : public ImageProcess
{
    Threshold(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    bool global=false;
    float globalLower = 0;
    float globalUpper = 1;
    bool binary=false;
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
    void Unload() override;
    void RecalculateKernel();
    int size=3;
    float sigma=1;
    int maxSize = 257;
    std::vector<float> kernel;
    GLuint kernelBuffer;

    bool shouldRecalculateKernel=true;
};

struct HalfToning : public ImageProcess
{
    HalfToning(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    void RecalculateKernel();
    
    float intensity=1;

    bool grayScale=true;
    
    glm::mat3 CalculateTransform(float theta);

    glm::vec3 rotation;
    glm::mat3 transformR;
    glm::mat3 transformG;
    glm::mat3 transformB;

    int size=5;
    std::vector<float> H;
    GLuint HBuffer;
    bool shouldRecalculateH=true;

    int maskType = 0;
};


struct Dithering : public ImageProcess
{
    Dithering(bool enabled=true);
    void SetUniforms() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;
    bool RenderGui() override;
    void Unload() override;
    
    AddNoise addNoise;
    Threshold threshold;

    GL_TextureFloat tmpTexture;
};

struct Erosion : public ImageProcess
{
    Erosion(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    void RecalculateKernel();
    int size=3;
    int subSize=1;
    int maxSize = 256;
    std::vector<float> kernel;
    
    enum class StructuringElementShape
    {
        Circle = 0,
        Diamond = 1,
        Line = 2,
        Octagon = 3,
        Square = 5
    };
    StructuringElementShape shape = StructuringElementShape::Circle;

    float rotation=0;

    GLuint kernelBuffer;
    bool shouldRecalculateKernel=true;
};

struct AddGradient : public ImageProcess
{
    AddGradient(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    void RecalculateKernel();

    struct ColorStop
    {
        glm::vec3 color;
        float t;
    };
    std::vector<ColorStop> colorStops;

    float rotation;
    glm::mat2 transformMatrix;

    GL_TextureFloat gradientTexture;
    std::vector<glm::vec4> gradientData;
};

struct Dilation : public ImageProcess
{
    Dilation(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    void RecalculateKernel();
    int size=3;
    int subSize=1;
    int maxSize = 256;
    std::vector<float> kernel;
    
    enum class StructuringElementShape
    {
        Circle = 0,
        Diamond = 1,
        Line = 2,
        Octagon = 3,
        Square = 5
    };
    StructuringElementShape shape = StructuringElementShape::Circle;

    float rotation=0;

    GLuint kernelBuffer;
    bool shouldRecalculateKernel=true;
};

struct AddImage : public ImageProcess
{
    AddImage(bool enabled=true, char* fileName="");
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;

    char fileName[128];
    GL_TextureFloat texture;
    bool filenameChanged=false;
    float multiplier=1;
    float aspectRatio;
};

struct MultiplyImage : public ImageProcess
{
    MultiplyImage(bool enabled=true, char* fileName="");
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;

    char fileName[128];
    GL_TextureFloat texture;
    bool filenameChanged=false;
    float multiplier=1;
};

struct GaussianPyramid : public ImageProcess
{
    GaussianPyramid(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;
    void CopyTexture(GLuint textureIn, GLuint textureOut, int width, int height);
    void Build(GLuint textureIn, int width, int height);

    int depth=5;
    std::vector<GL_TextureFloat> pyramid;
    std::vector<GL_TextureFloat> pyramidTmp;
    // GL_TextureFloat reconstructed;
    int currentWidth=-1;
    int currentHeight=-1;
    GaussianBlur gaussianBlur;

    bool depthChanged;

    int output=0;
};

struct LaplacianPyramid : public ImageProcess
{
    LaplacianPyramid(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;
    void SubtractTexture(GLuint textureA, GLuint textureB, int width, int height);
    void AddTexture(GLuint textureA, GLuint textureB, int width, int height);
    void ClearTexture(GLuint textureA, int width, int height);
    void Build(GLuint textureIn, int width, int height);

    GaussianPyramid gaussianPyramid;

    GLint addTextureShader;
    GLint clearTextureShader;
    
    bool outputReconstruction=false;
    int output=0;
};

struct PenDraw : public ImageProcess
{
    PenDraw(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    
    virtual bool MouseMove(float x, float y) override;
    virtual bool MousePressed() override;
    virtual bool MouseReleased() override;

    GL_TextureFloat paintTexture;
    std::vector<glm::vec4> paintData;
    std::vector<bool> paintedPixels;

    bool mousePressed=false;
    glm::vec2 previousMousPos = glm::vec2(-1);

    int radius=5;
    glm::vec3 color = glm::vec3(1,0,0);

    std::vector<glm::ivec2> contourPoints;

    bool selected=false;
};

struct HardComposite : public ImageProcess
{
    HardComposite(bool enabled=true, char* fileName="");
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    
    virtual bool MouseMove(float x, float y) override;
    virtual bool MousePressed() override;
    virtual bool MouseReleased() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;

    //Mask
    GL_TextureFloat maskTexture;
    std::vector<glm::vec4> maskData;
    GLint viewMaskShader;
    int radius=25;
    bool drawingMask=false;
    bool adding=true;

    //Source texture
    char fileName[128];
    GL_TextureFloat texture;
    bool filenameChanged=false;
    GLint compositeShader;

    //Weighted transition
    bool doBlur=false;
    GL_TextureFloat smoothedMaskTexture;
    SmoothingFilter smoothFilter;

    bool selected=false;

    bool mousePressed=false;
    glm::vec2 previousMousPos = glm::vec2(-1);

};

struct SeamCarvingResize : public ImageProcess
{
    SeamCarvingResize(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;

    float SeamCarvingResize::CalculateCostAt(glm::ivec2 position, std::vector<glm::vec4> &image, int width, int height);
    
    std::vector<glm::vec4> originalData;
    std::vector<float> gradient;
    std::vector<glm::vec4> imageData;
    std::vector<glm::vec4> debugData;
    
    std::vector<float> costs;
    std::vector<int> directions;
    
    std::vector<glm::ivec2> onSeam;

    int numIncreases=0;
    int numDecreases=0;

    struct Seam
    {
        std::vector<glm::ivec2> points;
    };
    std::vector<Seam> seams;

    bool debug=false;
    bool debugChanged=false;

    int numPerIterations=1;
    int iterations=0;

    // int resizedWidth;

    bool increase=false;
};

struct PatchInpainting : public ImageProcess
{
    PatchInpainting(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    
    virtual bool MouseMove(float x, float y) override;
    virtual bool MousePressed() override;
    virtual bool MouseReleased() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;
    bool RenderOutputGui() override;
    

    void PatchInpainting::ExtractPatch(std::vector<glm::vec4> &image, int imageWidth, int imageHeight, glm::ivec2 position, int size, std::vector<glm::vec4>& patch);
    float ComparePatches(std::vector<glm::vec4> &patch1, std::vector<glm::vec4> &patch2);
    std::vector<glm::ivec2> CalculateContour(int width, int height);

    //Mask
    GL_TextureFloat maskTexture;
    std::vector<glm::vec4> maskData;
    GLint viewMaskShader;
    bool drawingMask=false;
    bool adding=true;

    bool selected=false;

    bool mousePressed=false;
    glm::vec2 previousMousPos = glm::vec2(-1);

    int radius = 1;

    bool iterate=false;
    int iteration=0;
    int subIteration=0;
    glm::ivec2 patchCenter;
    int patchSize=4;

    bool searchWholeImage=false;
    glm::ivec2 searchWindowStart = glm::ivec2(200, 100);
    glm::ivec2 searchWindowEnd = glm::ivec2(300, 153);

    std::vector<glm::ivec2> points;
    std::vector<glm::vec4> textureData;
};

struct MultiResComposite : public ImageProcess
{
    MultiResComposite(bool enabled=true, char* fileName="");
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
    
    virtual bool MouseMove(float x, float y) override;
    virtual bool MousePressed() override;
    virtual bool MouseReleased() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height) override;

    void Reconstruct(GLuint textureOut, GLuint sourceTexture, GLuint destTexture, GLuint maskTexture, int width, int height);

    //Mask
    GL_TextureFloat maskTexture;
    std::vector<glm::vec4> maskData;
    GLint viewMaskShader;
    int radius=25;
    bool drawingMask=false;
    bool adding=true;
    GaussianPyramid maskPyramid;

    //Source texture
    char fileName[128];
    GL_TextureFloat texture;
    bool filenameChanged=false;
    LaplacianPyramid sourcePyramid;
    
    LaplacianPyramid destPyramid;

    int depth = 5;

    bool selected=false;

    bool mousePressed=false;
    glm::vec2 previousMousPos = glm::vec2(-1);

};

struct CurveGrading : public ImageProcess
{
    CurveGrading(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;

    Curve redCurve;
    Curve greenCurve;
    Curve blueCurve;

    bool renderRedCurve=true;
    bool renderGreenCurve=false;
    bool renderBlueCurve=false;
    
    GLuint redLut;
    GLuint greenLut;
    GLuint blueLut;
};

struct AddColor : public ImageProcess
{
    AddColor(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    glm::vec3 color;
};

struct LaplacianOfGaussian : public ImageProcess
{
    LaplacianOfGaussian(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Unload() override;
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
    void Unload() override;
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
    void Unload() override;

    void RecalculateKernel();
    int size=3;
    float sigma=1;
    int maxSize = 257;
    float threshold=0.1f;
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

struct ColorDistance : public ImageProcess
{
    ColorDistance(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    
    glm::vec3 color;
    float distance;
};

struct EdgeLinking : public ImageProcess
{
    EdgeLinking(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void Unload() override;
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

struct RegionGrow : public ImageProcess
{
    RegionGrow(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void Unload() override;

    glm::vec2 clickedPoint = glm::vec2(-1,-1);
    std::vector<glm::vec4> inputData;
    std::vector<glm::vec4> mask;
    std::vector<bool> tracker;
    float threshold=0.1f;

    enum class OutputType
    {
        AddColorToImage,
        Mask,
        Isolate
    };
    OutputType outputType; 
};

struct RegionProperties : public ImageProcess
{
    RegionProperties(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    bool RenderOutputGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void Unload() override;
    int GetStartIndex(glm::ivec2 b, glm::ivec2 c);

    struct Region
    {
        glm::uvec2 center;
        std::vector<glm::ivec2> points;
        struct
        {
            glm::ivec2 minBB;
            glm::ivec2 maxBB;
        } boundingBox;
        float perimeter=0;
        float area=0;
        float compactness=0;
        float circularity=0;

        float eigenValue1;
        float eigenValue2;
        
        glm::vec2 eigenVector1;
        glm::vec2 eigenVector2;
        
        bool renderGui=false;
        bool drawBB=false;
        bool DrawAxes=false;
    };
    std::vector<Region> regions;

    std::vector<glm::ivec2> directions = 
    {
        glm::ivec2(-1, 0),//0:Left 
        glm::ivec2(-1,-1),//1:top left
        glm::ivec2( 0,-1),//2:top
        glm::ivec2( 1,-1),//3:top right
        glm::ivec2( 1, 0),//4:right
        glm::ivec2( 1, 1),//5:bottom right
        glm::ivec2( 0, 1),//6:bottom
        glm::ivec2(-1, 1),//7:bottom left
    };

    struct point
    {
        glm::ivec2 b;
        glm::ivec2 c;
    };


    std::vector<glm::vec4> inputData;
    bool calculateSkeleton=true;
    bool shouldProcess=true;
};

struct ErrorDiffusionHalftoning : public ImageProcess
{
    ErrorDiffusionHalftoning(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void Unload() override;
    void RecalculateMask();
    std::vector<glm::vec4> inputData;
    bool shouldProcess=true;

    std::vector<float> mask;
    std::vector<glm::ivec2> maskIndices;

    bool grayScale=false;
    float threshold=0.5f;

    bool shouldRecalculateMask;
    int masktype=0;
};

struct KMeansCluster : public ImageProcess
{
    KMeansCluster(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void Unload() override;

    std::vector<glm::vec4> inputData;
    std::vector<glm::vec3> clusterPositions;
    std::vector<std::vector<int>> clusterMapping;


    enum class OutputMode
    {
        RandomColor=0,
        ClusterColor=1,
        GrayScale=2
    }; 

    OutputMode outputMode = OutputMode::ClusterColor;

    bool shouldProcess=true;

    int numClusters=4;    
};

struct SuperPixelsCluster : public ImageProcess
{
    SuperPixelsCluster(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void Unload() override;

    struct ClusterData
    {
        glm::vec3 color;
        glm::vec2 position;
    };

    std::vector<glm::vec4> inputData;
    std::vector<ClusterData> clusterPositions;
    std::vector<std::vector<int>> clusterMapping;

    bool shouldProcess=true;
    float C = 0.1f;
    int numClusters=500;

    enum class OutputMode
    {
        RandomColor=0,
        ClusterColor=1,
        GrayScale=2
    }; 

    OutputMode outputMode = OutputMode::ClusterColor;
};

struct HoughTransform : public ImageProcess
{
    HoughTransform(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    void Unload() override;

    CannyEdgeDetector *cannyEdgeDetector;
    bool cannyChanged=true;

    int hoodSize = 17;
    int houghSpaceSize=750;
    
    std::vector<glm::vec4> houghSpace;
    std::vector<glm::vec4> edgeData;
    std::vector<glm::vec4> linesData;
    std::vector<glm::vec4> inputData;

    GL_TextureFloat houghTexture;

    float threshold=266;

    bool addToImage=true;
    bool viewEdges=true;
    bool shouldProcess=true;
};

struct PolygonFitting : public ImageProcess
{
    PolygonFitting(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);

    int GetStartIndex(glm::ivec2 b, glm::ivec2 c);

    float threshold=1;

    std::vector<glm::vec4> inputData;
};

struct OtsuThreshold : public ImageProcess
{
    OtsuThreshold(bool enabled=true);
    void SetUniforms() override;
    bool RenderGui() override;
    void Process(GLuint textureIn, GLuint textureOut, int width, int height);
    float threshold=1;
    std::vector<glm::vec4> inputData;
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
    void Unload() override;
    bool color=true;
    
    GLuint lutBuffer;
    GLuint pdfBuffer;
    GLuint histogramBuffer, boundsBuffer;

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


    std::vector<std::complex<double>> textureDataComplexOutCorrectedRed;
    std::vector<std::complex<double>> textureDataComplexOutCorrectedGreen;
    std::vector<std::complex<double>> textureDataComplexOutCorrectedBlue;
    std::vector<std::complex<double>> textureDataComplexOutRed;
    std::vector<std::complex<double>> textureDataComplexOutGreen;
    std::vector<std::complex<double>> textureDataComplexOutBlue;
    std::vector<std::complex<double>> textureDataComplexInRed;
    std::vector<std::complex<double>> textureDataComplexInGreen;
    std::vector<std::complex<double>> textureDataComplexInBlue;

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
    
    // GL_TextureFloat texture;
    // GL_TextureFloat tmpTexture;
    

    ImageProcessStack imageProcessStack;

    bool shouldProcess=true;
    GLuint outTexture;

    bool firstFrame=true;

};