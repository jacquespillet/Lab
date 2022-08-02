#include "ImageLab.hpp"

#include "GL/glew.h"
#include <glm/gtx/quaternion.hpp>

#include "GL_Helpers/Util.hpp"
#include <fstream>
#include <sstream>
#include <random>

#include "imgui.h"

#include <complex>
#include <fftw3.h>
#include <stack>

#define MODE 2
#define DEBUG_INPAINTING 0

float Color2GrayScale(glm::vec3 color)
{
    return (color.r + color.g + color.b) * 0.33333f;
}

glm::vec4 GrayScale2Color(float grayScale, float alpha=0)
{
    return glm::vec4(grayScale,grayScale,grayScale, alpha);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Curve::BuildPath()
{
    path.resize(numEval);
    data.resize(numEval);

    int stepsPerCurve = numEval / (int)(controlPoints.size()-1);
    int inx=0;
    for(int i=0; i<controlPoints.size()-1; i++)
    {
        glm::vec2 start = controlPoints[i].point;
        glm::vec2 startControl = controlPoints[i].control;
        if(startControl.x < start.x)
        {
            glm::vec2 controlToPoint = start - startControl;
            startControl += controlToPoint*2;
        }

        glm::vec2 end = controlPoints[i+1].point;
        glm::vec2 endControl = controlPoints[i+1].control;
        if(endControl.x > end.x)
        {
            glm::vec2 controlToPoint = end - endControl;
            endControl += 2 * controlToPoint;
        }      
        double stepSize = 1 / (double)stepsPerCurve;
        double xu = 0.0 , yu = 0.0;
        for(int j = 0 ; j <stepsPerCurve ; j++)
        {
            double u = j * stepSize;
            xu = pow(1-u,3)*start.x+3*u*pow(1-u,2)*startControl.x+3*pow(u,2)*(1-u)*endControl.x
                +pow(u,3)*end.x;
            yu = pow(1-u,3)*start.y+3*u*pow(1-u,2)*startControl.y+3*pow(u,2)*(1-u)*endControl.y
                +pow(u,3)*end.y;
            data[inx] = (float)yu;
            path[inx++] = glm::vec2(xu, yu);
        }
    }       
}

float Curve::Evaluate(float t)
{
    t = (std::min)(1.0f, (std::max)(0.0f, t));
    int i = (int)std::floor(t * numEval);
    return path[i].y;
}

bool Curve::Render(ImDrawList* drawList, ImU32 curveColor)
{
    bool changed=false;

    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    origin.x = cursorPos.x; origin.y = cursorPos.y; 
    ImGuiIO& io = ImGui::GetIO();
    
    
    //If mouse released, reinit mouse state
    if(io.MouseReleased[0])
    {
        mouseState.movingControlPoint=false;
        mouseState.movingPoint=false;
        mouseState.index=-1;
    }

    //If we're moving a point, move it and its control
    if(mouseState.movingPoint)
    {
        controlPoints[mouseState.index].point = ScreenToGraph(glm::vec2(io.MousePos.x, io.MousePos.y));
        controlPoints[mouseState.index].control = controlPoints[mouseState.index].point - mouseState.diff;
        changed=true;
    }

    //If we're moving a control point
    if(mouseState.movingControlPoint)
    {
        controlPoints[mouseState.index].control = ScreenToGraph(glm::vec2(io.MousePos.x, io.MousePos.y));
        changed=true;
    }

    //Frame
    ImGui::PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), true);
    drawList->AddRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(255, 255, 255, 255));
    
    //Draw the points
    for(int i=0; i<controlPoints.size(); i++)
    {
        bool hoverPoint = false;
        bool hoverControlPoint = false;
        
        //Positions in screen space
        glm::vec2 pos = GraphToScreen(controlPoints[i].point);
        glm::vec2 controlPoint = GraphToScreen(controlPoints[i].control);
        
        ImU32 pointColor = IM_COL32(255,255,255,255);
        ImU32 controlColor = IM_COL32(128,128,128,255);
        
        //Check if we're hovering a point or a control point
        glm::vec2 mousePos = glm::vec2(io.MousePos.x, io.MousePos.y);
        if(glm::distance2(mousePos, pos) < pointRadius*pointRadius) hoverPoint=true;
        if(glm::distance2(mousePos, controlPoint) < pointRadius*pointRadius) hoverControlPoint=true;
        
        //Set color if hovering
        if(hoverPoint) pointColor = IM_COL32(255,0,0,255);
        if(hoverControlPoint) controlColor = IM_COL32(0,255,255,255);
        
        //If mouse pressed while hovering, set mouse state
        if(io.MouseClicked[0] && (hoverPoint || hoverControlPoint))
        {
            if(hoverPoint) mouseState.movingPoint=true;
            if(hoverControlPoint) mouseState.movingControlPoint=true;
            mouseState.index=i;
            mouseState.diff = controlPoints[i].point - controlPoints[i].control;
        }

        //Draw
        drawList->AddCircleFilled(ImVec2(pos.x, pos.y), pointRadius, pointColor);
        drawList->AddCircle(ImVec2(controlPoint.x, controlPoint.y), pointRadius, controlColor);
    }

    for(int i=0; i<controlPoints.size()-1; i++)
    {
        glm::vec2 start = GraphToScreen(controlPoints[i].point);
        glm::vec2 startControl = GraphToScreen(controlPoints[i].control);
        if(startControl.x < start.x)
        {
            glm::vec2 controlToPoint = start - startControl;
            startControl += controlToPoint*2;
        }

        glm::vec2 end = GraphToScreen(controlPoints[i+1].point);
        glm::vec2 endControl = GraphToScreen(controlPoints[i+1].control);
        if(endControl.x > end.x)
        {
            glm::vec2 controlToPoint = end - endControl;
            endControl += 2 * controlToPoint;
        }
        
        drawList->AddBezierCurve(ImVec2(start.x, start.y), ImVec2(startControl.x, startControl.y),
                                 ImVec2(endControl.x, endControl.y), ImVec2(end.x, end.y), 
                                 curveColor, 1);
        
        drawList->AddLine(ImVec2(start.x, start.y), ImVec2(startControl.x, startControl.y), IM_COL32(128,128,128,255), 1);
        drawList->AddLine(ImVec2(end.x, end.y), ImVec2(endControl.x, endControl.y), IM_COL32(128,128,128,255), 1);
    }

//Debug the evaluation
#if 0
	for(int i=0; i<path.size(); i++)
    {
        glm::vec2 p = GraphToScreen(path[i]);
        drawList->AddNgon(ImVec2(p.x, p.y), 3, IM_COL32(255,255,255,255), 3, 1);
    }
#endif


//Add control points
#if 0
    //Find position on the curve
    float mousePosGraphX = ScreenToGraph(glm::vec2(io.MousePos.x, 0)).x;
    int mouseInx = (int)(mousePosGraphX * (float)numEval);
    float mousePosGraphY = path[mouseInx].y;
    //Draw a circle at this position
    glm::vec2 posOnCurveScreen = GraphToScreen(glm::vec2(0, mousePosGraphY));
    drawList->AddCircleFilled(ImVec2(io.MousePos.x, posOnCurveScreen.y), 3, IM_COL32(164,164,164,255));
    
    if(io.MouseClicked[0] && !mouseState.movingControlPoint && !mouseState.movingPoint)
    {
        controlPoints.push_back(
            {
                glm::vec2(mousePosGraphX, mousePosGraphY),
                glm::vec2(mousePosGraphX, mousePosGraphY) + glm::vec2(0.3f, 0.3f)
            }
        );
        changed=true;
    }
#endif

    if(changed)
    {
#if 0
        std::sort(controlPoints.begin(), controlPoints.end(), [](const bezierPoint& a, const bezierPoint &b) -> bool 
        {
            return a.point.x < b.point.x;
        });
#endif
        BuildPath();
    }

    ImGui::PopClipRect();
    return changed;
}


//Takes normalized coordinates between 0 and 1
glm::vec2 Curve::GraphToScreen(glm::vec2 coord)
{
    glm::vec2 result(0,0);
    result.x  = origin.x + coord.x * size.x;
    result.y  = origin.y + size.y - coord.y * size.y;

    return result;
}

glm::vec2 Curve::ScreenToGraph(glm::vec2 coord)
{
    glm::vec2 result = coord - origin;
    result /= size;
    result = glm::clamp(result, glm::vec2(0), glm::vec2(1));
    result.y = 1 - result.y;
    return result;
}

float DistancePointLine(glm::vec2 v, glm::vec2 w, glm::vec2 p) {
  // Return minimum distance between line segment vw and point p
  const float l2 = glm::length2(v-w);  // i.e. |w-v|^2 -  avoid a sqrt
  if (l2 == 0.0) return glm::distance(p, v);   // v == w case
  
  const float t = std::max(0.0f, std::min(1.0f, glm::dot(p - v, w - v) / l2));
  const glm::vec2 projection = v + t * (w - v); 
  return glm::distance(p, projection);
}

void DrawLine(glm::ivec2 x0, glm::ivec2 x1, std::vector<glm::vec4> &image, int width, int height, glm::vec4 color)
{
    bool steep = false; 
    if (std::abs(x0.x-x1.x)<std::abs(x0.y-x1.y)) { 
        std::swap(x0.x, x0.y); 
        std::swap(x1.x, x1.y); 
        steep = true; 
    } 
    if (x0.x>x1.x) { 
        std::swap(x0.x, x1.x); 
        std::swap(x0.y, x1.y); 
    } 
    int dx = x1.x-x0.x; 
    int dy = x1.y-x0.y; 
    float derror = std::abs(dy/float(dx)); 
    float error = 0; 
    int y = x0.y; 
    for (int x=x0.x; x<=x1.x; x++) { 
        if (steep) { 
            if((x * width + y) >=0 && (x * width + y) < image.size()) image[x * width + y] = color;
        } else { 
            if((y * width + x) >= 0 && (y * width + x) < image.size())  image[y * width + x] = color;
        } 
        error += derror; 
        if (error>.5) { 
            y += (x1.y>x0.y?1:-1); 
            error -= 1.; 
        } 
    } 
}

ImageProcess::ImageProcess(std::string name, std::string shaderFileName, bool enabled) :name(name), shaderFileName(shaderFileName), enabled(enabled)
{
    if(shaderFileName != "") CreateComputeShader(shaderFileName, &shader);
}

void ImageProcess::SetUniforms()
{
    
}

bool ImageProcess::RenderGui()
{
    return false;
}

bool ImageProcess::RenderOutputGui()
{
    return false;
}

void ImageProcess::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
	glUseProgram(shader);
	SetUniforms();

	glUniform1i(glGetUniformLocation(shader, "textureIn"), 0); //program must be active
    glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	
    glUniform1i(glGetUniformLocation(shader, "textureOut"), 1); //program must be active
    glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
     
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void ImageProcessStack::Resize(int newWidth, int newHeight)
{
    {
        if(tex1.loaded) tex1.Unload();
        if(tex0.loaded) tex0.Unload();
        
        TextureCreateInfo tci = {};
        tci.minFilter = GL_NEAREST;
        tci.magFilter = GL_NEAREST;
        tex1 = GL_TextureFloat(newWidth, newHeight, tci);
        tex0 = GL_TextureFloat(newWidth, newHeight, tci);

        this->width = newWidth;
        this->height = newHeight;
    }
}


ImageProcessStack::ImageProcessStack()
{
    struct HistogramBuffer 
    {
        glm::ivec4 histogramGray[255];
    } histogramData;
    struct BoundsBuffer 
    {
        // glm::ivec4 minBound;
        glm::ivec4 maxBound;
    } boundsData;

    glGenBuffers(1, (GLuint*)&histogramBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(HistogramBuffer), &histogramData, GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     

    glGenBuffers(1, (GLuint*)&boundsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, boundsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BoundsBuffer), &boundsData, GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, boundsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     

    TextureCreateInfo tci = {};
    tci.minFilter = GL_LINEAR;
    tci.magFilter = GL_LINEAR;
    histogramTexture = GL_TextureFloat(256, 256, tci);

    CreateComputeShader("shaders/Histogram.glsl", &histogramShader);
    CreateComputeShader("shaders/ResetHistogram.glsl", &resetHistogramShader);
    CreateComputeShader("shaders/RenderHistogram.glsl", &renderHistogramShader);
    CreateComputeShader("shaders/ClearTexture.glsl", &clearTextureShader);

    Resize(width, height);
}

void ImageProcessStack::AddProcess(ImageProcess* imageProcess)
{
    imageProcess->imageProcessStack = this;
    imageProcesses.push_back(imageProcess);

    changed=true;
}



void ImageProcessStack::RenderHistogram()
{
    glUseProgram(renderHistogramShader);

    glUniform1i(glGetUniformLocation(renderHistogramShader, "histogramR"), (int)histogramR); //program must be active
    glUniform1i(glGetUniformLocation(renderHistogramShader, "histogramG"), (int)histogramG); //program must be active
    glUniform1i(glGetUniformLocation(renderHistogramShader, "histogramB"), (int)histogramB); //program must be active
    glUniform1i(glGetUniformLocation(renderHistogramShader, "histogramGray"), (int)histogramGray); //program must be active

    glUniform1i(glGetUniformLocation(renderHistogramShader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, histogramTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, boundsBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boundsBuffer);

    glDispatchCompute((histogramTexture.width / 32) + 1, (histogramTexture.height / 32) + 1, 1);
    glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);        
}

GLuint ImageProcessStack::Process()
{
    //Clear the input texture
    glUseProgram(clearTextureShader);
    glUniform1i(glGetUniformLocation(clearTextureShader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, tex0.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(tex0.width/32+1, tex0.height/32+1, 1);
    glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);     

    std::vector<ImageProcess*> activeProcesses;
    activeProcesses.reserve(imageProcesses.size());
    for(int i=0; i<imageProcesses.size(); i++)
    {
        if(imageProcesses[i]->enabled) activeProcesses.push_back(imageProcesses[i]);
    }

    for(int i=0; i<activeProcesses.size(); i++)
    {
        if(i==0)
        {
            activeProcesses[i]->Process(
                                    tex0.glTex,
                                    tex1.glTex,
                                    width, 
                                    height);
        }
        else
        {
            bool pairPass = i % 2 == 0;
            activeProcesses[i]->Process(
                                    pairPass ? tex0.glTex : tex1.glTex, 
                                    pairPass ? tex1.glTex : tex0.glTex, 
                                    width, 
                                    height);
        }
    }


    GLuint resultTexture = (activeProcesses.size() % 2 == 0) ? tex0.glTex : tex1.glTex;
    if(activeProcesses.size() == 0)
    {
        resultTexture = tex0.glTex;
    }
    
    //Pass to reset histograms
    {
        glUseProgram(resetHistogramShader);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boundsBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boundsBuffer);

        glDispatchCompute(1, 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);        
    }

    //Pass to calculate histograms
    {
        glUseProgram(histogramShader);

        glUniform1i(glGetUniformLocation(histogramShader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, resultTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boundsBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boundsBuffer);
	
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);        
    }
    
    
    RenderHistogram();

    //Read back from texture
    if(outputImage.size() != width * height) outputImage.resize(width*height); 
    glBindTexture(GL_TEXTURE_2D, resultTexture);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    outputImage.data());
    glBindTexture(GL_TEXTURE_2D, 0);    

    return resultTexture;
}

bool ImageProcessStack::RenderGUI()
{
    changed=false;
    
    ImGui::Image((ImTextureID)histogramTexture.glTex, ImVec2(255, 255));
    changed |= ImGui::Checkbox("R", &histogramR); ImGui::SameLine(); 
    changed |= ImGui::Checkbox("G", &histogramG); ImGui::SameLine();
    changed |= ImGui::Checkbox("B", &histogramB); ImGui::SameLine();
    changed |= ImGui::Checkbox("Gray", &histogramGray);
    
    ImGui::Button("+");
    if (ImGui::BeginPopupContextItem("HBEFKWJJNFOIKWEJNF", 0))
    {
        // ImGui::Button("HELLO");
        
        if(ImGui::Button("ColorContrastStretch")) AddProcess(new ColorContrastStretch(true));
        if(ImGui::Button("GrayScaleContrastStretch")) AddProcess(new GrayScaleContrastStretch(true));
        if(ImGui::Button("Negative")) AddProcess(new Negative(true));
        if(ImGui::Button("Threshold")) AddProcess(new Threshold(true));
        if(ImGui::Button("Quantization")) AddProcess(new Quantization(true));
        if(ImGui::Button("Transform")) AddProcess(new Transform(true));
        if(ImGui::Button("Resampling")) AddProcess(new Resampling(true));
        if(ImGui::Button("AddNoise")) AddProcess(new AddNoise(true));
        if(ImGui::Button("AddGradient")) AddProcess(new AddGradient(true));
        if(ImGui::Button("SmoothingFilter")) AddProcess(new SmoothingFilter(true));
        if(ImGui::Button("SharpenFilter")) AddProcess(new SharpenFilter(true));
        if(ImGui::Button("SobelFilter")) AddProcess(new SobelFilter(true));
        if(ImGui::Button("MedianFilter")) AddProcess(new MedianFilter(true));
        if(ImGui::Button("MinMaxFilter")) AddProcess(new MinMaxFilter(true));
        if(ImGui::Button("ArbitraryFilter")) AddProcess(new ArbitraryFilter(true));
        if(ImGui::Button("GaussianBlur")) AddProcess(new GaussianBlur(true));
        if(ImGui::Button("GammaCorrection")) AddProcess(new GammaCorrection(true));
        if(ImGui::Button("Equalize")) AddProcess(new Equalize(true));        
        if(ImGui::Button("FFT Blur")) AddProcess(new FFTBlur(true));        
        if(ImGui::Button("Gradient")) AddProcess(new Gradient(true));        
        if(ImGui::Button("LaplacianOfGaussian")) AddProcess(new LaplacianOfGaussian(true));        
        if(ImGui::Button("DifferenceOfGaussians")) AddProcess(new DifferenceOfGaussians(true));        
        if(ImGui::Button("CannyEdgeDetector")) AddProcess(new CannyEdgeDetector(true));        
        if(ImGui::Button("EdgeLinking")) AddProcess(new EdgeLinking(true));        
        if(ImGui::Button("HoughTransform")) AddProcess(new HoughTransform(true));        
        if(ImGui::Button("PolygonFitting")) AddProcess(new PolygonFitting(true));        
        if(ImGui::Button("ColorDistance")) AddProcess(new ColorDistance(true));        
        if(ImGui::Button("AddImage")) AddProcess(new AddImage(true));        
        if(ImGui::Button("AddColor")) AddProcess(new AddColor(true));        
        if(ImGui::Button("MultiplyImage")) AddProcess(new MultiplyImage(true));        
        if(ImGui::Button("CurveGrading")) AddProcess(new CurveGrading(true));
        if(ImGui::Button("OtsuThreshold")) AddProcess(new OtsuThreshold(true));
        if(ImGui::Button("LocalThreshold")) AddProcess(new LocalThreshold(true));
        if(ImGui::Button("RegionGrow")) AddProcess(new RegionGrow(true));
        if(ImGui::Button("KMeansCluster")) AddProcess(new KMeansCluster(true));
        if(ImGui::Button("SuperPixelsCluster")) AddProcess(new SuperPixelsCluster(true));
        if(ImGui::Button("Erosion")) AddProcess(new Erosion(true));
        if(ImGui::Button("Dilation")) AddProcess(new Dilation(true));
        if(ImGui::Button("RegionProperties")) AddProcess(new RegionProperties(true));
        if(ImGui::Button("HalfToning")) AddProcess(new HalfToning(true));
        if(ImGui::Button("Dithering")) AddProcess(new Dithering(true));
        if(ImGui::Button("ErrorDiffusionHalftoning")) AddProcess(new ErrorDiffusionHalftoning(true));
        if(ImGui::Button("PenDraw")) AddProcess(new PenDraw(true));
        if(ImGui::Button("HardComposite")) AddProcess(new HardComposite(true));
        if(ImGui::Button("GaussianPyramid")) AddProcess(new GaussianPyramid(true));
        if(ImGui::Button("LaplacianPyramid")) AddProcess(new LaplacianPyramid(true));
        if(ImGui::Button("MultiResComposite")) AddProcess(new MultiResComposite(true));
        if(ImGui::Button("PatchInpainting")) AddProcess(new PatchInpainting(true));

        ImGui::EndPopup();
    }

	ImGui::Separator();
    ImGui::Text("Post Processes");
	ImGui::Separator();

    static ImageProcess *selectedImageProcess=nullptr;
    for (int n = 0; n < imageProcesses.size(); n++)
    {
        ImageProcess *item = imageProcesses[n];
        
        ImGui::PushID(n * 2 + 0);
        changed |= ImGui::Checkbox("", &imageProcesses[n]->enabled);
        ImGui::PopID();
        ImGui::SameLine();
        bool isSelected=false;
        
        
        ImGui::PushID(n * 2 + 1);
        if(ImGui::Selectable(item->name.c_str(), &isSelected))
        {
            selectedImageProcess = item;
        }
        ImGui::PopID();

        if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
        {
            int n_next = n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
            if (n_next >= 0 && n_next < imageProcesses.size())
            {
                imageProcesses[n] = imageProcesses[n_next];
                imageProcesses[n_next] = item;
                ImGui::ResetMouseDragDelta();
            }
        }

        if (isSelected)
            ImGui::SetItemDefaultFocus();    
    }    

	ImGui::Separator();
    
    if(selectedImageProcess != nullptr)
    {
        changed |= selectedImageProcess->RenderGui();
    }

    return changed;
}

void ImageProcessStack::Unload()
{
    glDeleteProgram(histogramShader);
    glDeleteProgram(resetHistogramShader);
    glDeleteProgram(renderHistogramShader);
    
    glDeleteBuffers(1, &boundsBuffer);
    glDeleteBuffers(1, &histogramBuffer);
    histogramTexture.Unload();
    for(int i=0; i<imageProcesses.size(); i++)
    {
        imageProcesses[i]->Unload();
        delete imageProcesses[i];
    }
}

//
//------------------------------------------------------------------------
GrayScaleContrastStretch::GrayScaleContrastStretch(bool enabled) : ImageProcess("GrayScaleContrastStretch", "shaders/GrayScaleContrastStretch.glsl", enabled)
{
}

void GrayScaleContrastStretch::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "lowerBound"), lowerBound);
    glUniform1f(glGetUniformLocation(shader, "upperBound"), upperBound);
}

bool GrayScaleContrastStretch::RenderGui()
{
    bool changed=false;
    ImGui::SetNextItemWidth(255); changed |= ImGui::SliderFloat("lowerBound", &lowerBound, 0, 1);
    ImGui::SetNextItemWidth(255); changed |= ImGui::SliderFloat("upperBound", &upperBound, 0, 1);
    

    return changed;
}
//

//
//------------------------------------------------------------------------
CurveGrading::CurveGrading(bool enabled) : ImageProcess("CurveGrading", "shaders/CurveGrading.glsl", enabled)
{
    redCurve.controlPoints = 
    {
        {glm::vec2(0,0), glm::vec2(0.25,0.25)},
        // {glm::vec2(0.25,0.5), glm::vec2(0.70,0.70)},
        {glm::vec2(1,1), glm::vec2(0.75,0.75)},
    };
    greenCurve.controlPoints = redCurve.controlPoints;
    blueCurve.controlPoints = redCurve.controlPoints;

    redCurve.BuildPath();
    greenCurve.BuildPath();
    blueCurve.BuildPath();
    
    glGenBuffers(1, (GLuint*)&redLut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, redLut);
	glBufferData(GL_SHADER_STORAGE_BUFFER, redCurve.data.size() * sizeof(float), redCurve.data.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, redLut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
    
    glGenBuffers(1, (GLuint*)&greenLut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, greenLut);
	glBufferData(GL_SHADER_STORAGE_BUFFER, greenCurve.data.size() * sizeof(float), greenCurve.data.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, greenLut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
    
    glGenBuffers(1, (GLuint*)&blueLut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, blueLut);
	glBufferData(GL_SHADER_STORAGE_BUFFER, blueCurve.data.size() * sizeof(float), blueCurve.data.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, blueLut);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
}

void CurveGrading::SetUniforms()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, redLut);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, redLut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, greenLut);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, greenLut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, blueLut);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, blueLut);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 

}

bool CurveGrading::RenderGui()
{
    bool changed=false;
    // ImGui::SetNextItemWidth(255); changed |= ImGui::SliderFloat("lowerBound", &lowerBound, 0, 1);
    // ImGui::SetNextItemWidth(255); changed |= ImGui::SliderFloat("upperBound", &upperBound, 0, 1);
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    int width = 128;
    int height = 128;
    
    bool redChanged=false;
    bool greenChanged=false;
    bool blueChanged=false;

    redCurve.size = glm::vec2(width,height);
    if(renderRedCurve) redChanged |= redCurve.Render(drawList, IM_COL32(255, 0, 0, 255));
    greenCurve.size = glm::vec2(width,height);
    if(renderGreenCurve) greenChanged |= greenCurve.Render(drawList, IM_COL32(0, 255, 0, 255));
    redCurve.size = glm::vec2(width,height);
    if(renderBlueCurve) blueChanged |= blueCurve.Render(drawList, IM_COL32(0, 0, 255, 255));

    changed |= redChanged;
    changed |= greenChanged;
    changed |= blueChanged;

    ImGui::InvisibleButton("Padding", ImVec2((float)width, (float)height));
    ImGui::Checkbox("Red", &renderRedCurve); ImGui::SameLine();
    ImGui::Checkbox("Green", &renderGreenCurve); ImGui::SameLine();
    ImGui::Checkbox("Blue", &renderBlueCurve);

    if(redChanged)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, redLut);
        glBufferData(GL_SHADER_STORAGE_BUFFER, redCurve.data.size() * sizeof(float), redCurve.data.data(), GL_DYNAMIC_COPY); 
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    if(greenChanged)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, greenLut);
        glBufferData(GL_SHADER_STORAGE_BUFFER, greenCurve.data.size() * sizeof(float), greenCurve.data.data(), GL_DYNAMIC_COPY); 
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    if(blueChanged)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, blueLut);
        glBufferData(GL_SHADER_STORAGE_BUFFER, blueCurve.data.size() * sizeof(float), blueCurve.data.data(), GL_DYNAMIC_COPY); 
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
    
    return changed;
}

void CurveGrading::Unload() 
{

}
//

//
//------------------------------------------------------------------------
Equalize::Equalize(bool enabled) : ImageProcess("Equalize", "shaders/Equalize.glsl", enabled)
{
    std::vector<glm::ivec4> lutData(255);
    glGenBuffers(1, (GLuint*)&lutBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lutBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, lutData.size() * sizeof(glm::ivec4), lutData.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lutBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 

    std::vector<glm::vec4> pdfData(255);
    glGenBuffers(1, (GLuint*)&pdfBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pdfBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, pdfData.size() * sizeof(glm::vec4), pdfData.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pdfBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 

    struct HistogramBuffer 
    {
        glm::ivec4 histogramGray[255];
    } histogramData;
    struct BoundsBuffer 
    {
        // glm::ivec4 minBound;
        glm::ivec4 maxBound;
    } boundsData;

    glGenBuffers(1, (GLuint*)&histogramBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(HistogramBuffer), &histogramData, GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     

    glGenBuffers(1, (GLuint*)&boundsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, boundsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BoundsBuffer), &boundsData, GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, boundsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); 

    CreateComputeShader("shaders/EqualizeBuildPdf.glsl", &buildPdfShader);
    CreateComputeShader("shaders/EqualizeBuildLut.glsl", &buildLutShader);
    CreateComputeShader("shaders/ResetHistogram.glsl", &resetHistogramShader);
    CreateComputeShader("shaders/Histogram.glsl", &histogramShader);
}

void Equalize::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "equalizeColor"), (int)color); //program must be active
}

void Equalize::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    
    //Reset histogram
    {
        glUseProgram(resetHistogramShader);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boundsBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boundsBuffer);
        glDispatchCompute(1, 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);        
    }

    //Pass to calculate histogram of the input image
    {
        glUseProgram(histogramShader);

        glUniform1i(glGetUniformLocation(histogramShader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boundsBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boundsBuffer);
	
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);        
    }
    
    
    { //build lut
        glUseProgram(buildLutShader);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, histogramBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, histogramBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pdfBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, pdfBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lutBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lutBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        glUniform1f(glGetUniformLocation(buildLutShader, "textureSize"), (float)(width * height)); //program must be active
        
        glDispatchCompute(1, 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);            
    }

    
    { //correction pass
        glUseProgram(shader);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lutBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lutBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        SetUniforms();

        glUniform1i(glGetUniformLocation(shader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(shader, "textureOut"), 1); //program must be active
        glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);    
    }
}

bool Equalize::RenderGui()
{
    bool changed=false;
    changed |= ImGui::Checkbox("Color", &color);
    return changed;
}

void Equalize::Unload()
{
    glDeleteBuffers(1, &lutBuffer);
    glDeleteBuffers(1, &pdfBuffer);
    glDeleteBuffers(1, &histogramBuffer);
    glDeleteBuffers(1, &boundsBuffer);
    
    
    glDeleteProgram(resetHistogramShader);
    glDeleteProgram(histogramShader);
    glDeleteProgram(buildPdfShader);
    glDeleteProgram(buildLutShader);    
}

//

//
//------------------------------------------------------------------------
ColorContrastStretch::ColorContrastStretch(bool enabled) : ImageProcess("ColorContrastStretch", "shaders/ColorContrastStrech.glsl", enabled)
{
}

void ColorContrastStretch::SetUniforms()
{
    if(global)
    {
        glUniform3f(glGetUniformLocation(shader, "lowerBound"), globalLowerBound, globalLowerBound, globalLowerBound);
        glUniform3f(glGetUniformLocation(shader, "upperBound"), globalUpperBound, globalUpperBound, globalUpperBound);
    }
    else
    {
        glUniform3fv(glGetUniformLocation(shader, "lowerBound"), 1, glm::value_ptr(lowerBound));
        glUniform3fv(glGetUniformLocation(shader, "upperBound"), 1, glm::value_ptr(upperBound));
    }
}

bool ColorContrastStretch::RenderGui()
{
    bool changed=false;
 
    changed |= ImGui::Checkbox("Global", &global);

    if(global)
    {
        changed |= ImGui::DragFloatRange2("Range", &globalLowerBound, &globalUpperBound, 0.01f, 0, 1);
    }
    else
    {
        changed |= ImGui::DragFloatRange2("Range Red", &lowerBound.x, &upperBound.x, 0.01f, 0, 1);
        changed |= ImGui::DragFloatRange2("Range Gren", &lowerBound.y, &upperBound.y, 0.01f, 0, 1);
        changed |= ImGui::DragFloatRange2("Range Blue", &lowerBound.z, &upperBound.z, 0.01f, 0, 1);
    }

    
    return changed;    
}
//

//
//------------------------------------------------------------------------
Negative::Negative(bool enabled) : ImageProcess("Negative", "shaders/Negative.glsl", enabled)
{}

void Negative::SetUniforms()
{}

bool Negative::RenderGui()
{ return false;}
//

//
//------------------------------------------------------------------------
LocalThreshold::LocalThreshold(bool enabled) : ImageProcess("LocalThreshold", "shaders/LocalThreshold.glsl", enabled)
{
    
}

void LocalThreshold::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "size"), size);  
    glUniform1f(glGetUniformLocation(shader, "a"), a);  
    glUniform1f(glGetUniformLocation(shader, "b"), b);  
}

bool LocalThreshold::RenderGui()
{ 
    bool changed=false;
    changed |= ImGui::SliderInt("Size", &size, 0, 33);
    changed |= ImGui::DragFloat("A", &a, 0.001f);
    changed |= ImGui::DragFloat("B", &b, 0.001f);
    return changed;
}
//

//
//------------------------------------------------------------------------
Threshold::Threshold(bool enabled) : ImageProcess("Threshold", "shaders/Threshold.glsl", enabled)
{}

void Threshold::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "global"), (int)global);
    glUniform1i(glGetUniformLocation(shader, "binary"), (int)binary);
    if(global)
    {
        glUniform3f(glGetUniformLocation(shader, "lowerBound"), globalLower, globalLower, globalLower);
        glUniform3f(glGetUniformLocation(shader, "upperBound"), globalUpper, globalUpper, globalUpper);
    }
    else
    {
        glUniform3fv(glGetUniformLocation(shader, "lowerBound"), 1, glm::value_ptr(lower));
        glUniform3fv(glGetUniformLocation(shader, "upperBound"), 1, glm::value_ptr(upper));
    }
}

bool Threshold::RenderGui()
{
    bool changed=false;
    changed |= ImGui::Checkbox("Binary", &binary);
    changed |= ImGui::Checkbox("Global", &global);
    if(global)
    {
        changed |= ImGui::SliderFloat("Lower bound", &globalLower, 0, 1);
        changed |= ImGui::SliderFloat("upper bound", &globalUpper, 0, 1);
    }
    else
    {
        changed |= ImGui::SliderFloat3("Lower bound", &lower[0], 0, 1);
        changed |= ImGui::SliderFloat3("upper bound", &upper[0], 0, 1);
    }

    return changed;
}
//

//
//------------------------------------------------------------------------
Quantization::Quantization(bool enabled) : ImageProcess("Quantization", "shaders/Quantization.glsl", enabled)
{}

void Quantization::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "levels"), numLevels);
}

bool Quantization::RenderGui()
{
    bool changed=false;
    changed |= ImGui::SliderInt("Levels", &numLevels, 2, 255);
    return changed;
}
//

//
//------------------------------------------------------------------------
Transform::Transform(bool enabled) : ImageProcess("Transform", "shaders/Transform.glsl", enabled)
{}

void Transform::SetUniforms()
{
    glm::mat3 transform(1);

    float cosTheta = cos(glm::radians(theta));
    float sinTheta = sin(glm::radians(theta));

    glm::mat3 rotationMatrix(1);
    rotationMatrix[0][0] = cosTheta;
    rotationMatrix[1][0] = -sinTheta;
    rotationMatrix[0][1] = sinTheta;
    rotationMatrix[1][1] = cosTheta;

    glm::mat3 translationMatrix(1);
    translationMatrix[2][0] = translation.x;
    translationMatrix[2][1] = translation.y;

    glm::mat3 shearMatrix(1);
    shearMatrix[1][0] = shear.x;
    shearMatrix[0][1] = shear.y;

    glm::mat3 scaleMatrix(1);
    scaleMatrix[0][0] = scale.x;
    scaleMatrix[1][1] = scale.y;
    
    
    transform = rotationMatrix * shearMatrix * scaleMatrix * translationMatrix;
    transform = glm::inverse(transform);    

    glUniformMatrix3fv(glGetUniformLocation(shader, "transformMatrix"), 1, GL_FALSE, glm::value_ptr(transform));
    glUniform1i(glGetUniformLocation(shader, "interpolation"), interpolation);
    
    glUniform1i(glGetUniformLocation(shader, "flipX"), (int)flipX);
    glUniform1i(glGetUniformLocation(shader, "flipY"), (int)flipY);
}

bool Transform::RenderGui()
{
    bool changed=false;
    
    changed |= ImGui::DragFloat2("Translation", &translation[0], 0.5f);
    changed |= ImGui::DragFloat("Rotation", &theta, 0.2f);
    changed |= ImGui::DragFloat2("Scale", &scale[0], 0.1f);
    changed |= ImGui::DragFloat2("Shear", &shear[0], 0.01f);

    changed |= ImGui::Combo("Render Mode", &interpolation, "Nearest\0Bilinear\0Bicubic\0\0");

    changed |= ImGui::Checkbox("Flip X", &flipX);
    changed |= ImGui::Checkbox("Flip Y", &flipY);

    return changed;
}
//

//
//------------------------------------------------------------------------
Resampling::Resampling(bool enabled) : ImageProcess("Resampling", "shaders/Resampling.glsl", enabled)
{}

void Resampling::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "pixelSize"), pixelSize);
}

bool Resampling::RenderGui()
{
    bool changed=false;
    changed |= ImGui::SliderInt("Pixel Size", &pixelSize, 1, 255);
    return changed;
}
//

//
//------------------------------------------------------------------------
AddNoise::AddNoise(bool enabled) : ImageProcess("AddNoise", "shaders/AddNoise.glsl", enabled)
{}

void AddNoise::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "density"), density);
    glUniform1f(glGetUniformLocation(shader, "intensity"), intensity);
    glUniform1i(glGetUniformLocation(shader, "randomColor"), (int)randomColor);
}

bool AddNoise::RenderGui()
{
    bool changed=false;
    changed |= ImGui::DragFloat("Density", &density, 0.001f);
    changed |= ImGui::DragFloat("Intensity", &intensity, 0.001f);
    changed |= ImGui::Checkbox("Random Color", &randomColor);

    return changed;
}
//

//
//------------------------------------------------------------------------
SmoothingFilter::SmoothingFilter(bool enabled) : ImageProcess("SmoothingFilter", "shaders/SmoothingFilter.glsl", enabled)
{}

void SmoothingFilter::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "size"), size);
}

bool SmoothingFilter::RenderGui()
{
    bool changed=false;
    changed |= ImGui::SliderInt("Size", &size, 0, 128);
    return changed;
}
//

//
//------------------------------------------------------------------------
SharpenFilter::SharpenFilter(bool enabled) : ImageProcess("SharpenFilter", "shaders/SharpenFilter.glsl", enabled)
{}

void SharpenFilter::SetUniforms()
{
}

bool SharpenFilter::RenderGui()
{
    return false;
}
//

//
//------------------------------------------------------------------------
MedianFilter::MedianFilter(bool enabled) : ImageProcess("MedianFilter", "shaders/MedianFilter.glsl", enabled)
{}

void MedianFilter::SetUniforms()
{
}

bool MedianFilter::RenderGui()
{
    return false;
}
//

//
//------------------------------------------------------------------------
SobelFilter::SobelFilter(bool enabled) : ImageProcess("SobelFilter", "shaders/SobelFilter.glsl", enabled)
{}

void SobelFilter::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "vertical"), (int)vertical);
}

bool SobelFilter::RenderGui()
{
    bool changed=false;
    changed |= ImGui::Checkbox("Vertical", &vertical);
    return changed;
}
//

//
//------------------------------------------------------------------------
MinMaxFilter::MinMaxFilter(bool enabled) : ImageProcess("MinMaxFilter", "shaders/MinMaxFilter.glsl", enabled)
{}

void MinMaxFilter::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "size"), size);
    glUniform1i(glGetUniformLocation(shader, "doMin"), doMin);
}

bool MinMaxFilter::RenderGui()
{
    bool changed=false;
    changed |= ImGui::Checkbox("Min", &doMin);
    changed |= ImGui::SliderInt("size", &size, 1, 32);
    return changed;
}
//

//
//------------------------------------------------------------------------
Gradient::Gradient(bool enabled) : ImageProcess("Gradient", "shaders/Gradient.glsl", enabled)
{

}

void Gradient::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "renderMode"), (int)renderMode);
    
    glUniform1f(glGetUniformLocation(shader, "minMag"), minMag);
    glUniform1f(glGetUniformLocation(shader, "maxMag"), maxMag);
    glUniform1f(glGetUniformLocation(shader, "angleRange"), angleRange);
    glUniform1f(glGetUniformLocation(shader, "filterAngle"), angle);
}

bool Gradient::RenderGui()
{
    bool changed=false;
    changed |= ImGui::Combo("Output ", (int*)&renderMode, "Magnitude\0GradientX\0GradientY\0GradientXY\0Angle\0\0");

    changed |= ImGui::DragFloatRange2("Magnitude Threshold", &minMag, &maxMag, 0.01f, 0, 1);
    changed |= ImGui::SliderFloat("Angle Range", &angleRange, 0, PI);
    changed |= ImGui::SliderFloat("Angle", &angle, 0, PI);
    //changed |= ImGui::DragFloatRange2("Angle Threshold", &minAngle, &maxAngle, 0.01f, 0, PI);
    return changed;
}
//

//
//------------------------------------------------------------------------
GaussianBlur::GaussianBlur(bool enabled) : ImageProcess("GaussianBlur", "shaders/GaussianBlur.glsl", enabled)
{
    kernel.resize(maxSize * maxSize);
    glGenBuffers(1, (GLuint*)&kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     
}

void GaussianBlur::RecalculateKernel()
{
    int halfSize = (int)std::floor(size / 2.0f);
    double r, s = 2.0 * sigma * sigma;
    double sum = 0.0;
    
    for (int x = -halfSize; x <= halfSize; x++) {
        for (int y = -halfSize; y <= halfSize; y++) {
            int flatInx = (y + halfSize) * size + (x + halfSize);
            r = (float)sqrt(x * x + y * y);
			double num = (exp(-(r * r) / s));
			double denom = (PI * s);
            kernel[flatInx] =  (float)(num / denom);
            sum += kernel[flatInx];
        }
    }

    // normalising the Kernel
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            int flatInx = (i) * size + (j);
            kernel[flatInx] /= (float)sum;
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    shouldRecalculateKernel=false;
}

void GaussianBlur::SetUniforms()
{
    if(shouldRecalculateKernel) RecalculateKernel();

    glUniform1i(glGetUniformLocation(shader, "size"), size);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
}

bool GaussianBlur::RenderGui()
{
    bool changed=false;
    shouldRecalculateKernel |= ImGui::SliderInt("Size", &size, 1, maxSize);
    shouldRecalculateKernel |= ImGui::SliderFloat("Sigma", &sigma, 1, 10);

    changed |= shouldRecalculateKernel;
    return changed;
}

void GaussianBlur::Unload()
{
    glDeleteBuffers(1, &kernelBuffer);
}
//

//
//------------------------------------------------------------------------
HalfToning::HalfToning(bool enabled) : ImageProcess("HalfToning", "shaders/HalfToning.glsl", enabled)
{
    H.resize(5 * 5);
    glGenBuffers(1, (GLuint*)&HBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, HBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, H.size() * sizeof(float), H.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, HBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     
}

void HalfToning::RecalculateKernel()
{
    if(maskType==0)
    {
        float t = 1 / 255.0f;
        H = 
        {
            t * 80,    t * 170,   t * 240,   t * 200,   t * 10,
            t * 40,    t * 60,    t * 150,   t * 90,   t * 10,
            t * 140,   t * 210,   t * 250,   t * 220,   t * 30,
            t * 120,   t * 190,   t * 230,   t * 180,   t * 0,
            t * 20,    t * 100,   t * 160,   t * 50,   t * 30
        };
        size = 5;
    }
    else if(maskType==1)
    {
        float t = 16 / 255.0f;
        H = 
        {
            t * 1,  t * 9,  t * 3,  t * 11,
            t * 13, t * 5,  t * 15, t * 7,
            t * 4,  t * 12, t * 2,  t * 10,
            t * 16, t * 8,  t * 14, t * 16
        };
        size = 4;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, HBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, H.size() * sizeof(float), H.data(), GL_DYNAMIC_COPY); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    shouldRecalculateH=false;
}

glm::mat3 HalfToning::CalculateTransform(float theta)
{
    glm::mat3 transform;
    float cosTheta = cos(glm::radians(theta));
    float sinTheta = sin(glm::radians(theta));

    glm::mat3 rotationMatrix(1);
    rotationMatrix[0][0] = cosTheta;
    rotationMatrix[1][0] = -sinTheta;
    rotationMatrix[0][1] = sinTheta;
    rotationMatrix[1][1] = cosTheta;
    
    transform = rotationMatrix;
    transform = glm::inverse(transform);    
    return transform;

}

void HalfToning::SetUniforms()
{
    if(shouldRecalculateH) RecalculateKernel();

    if(grayScale)
    {
        transformR = CalculateTransform(rotation.r);
        glUniformMatrix3fv(glGetUniformLocation(shader, "rotation"), 1, GL_FALSE, glm::value_ptr(transformR));
    }
    else
    {
        transformR = CalculateTransform(rotation.r);
        transformG = CalculateTransform(rotation.g);
        transformB = CalculateTransform(rotation.b);

        glUniformMatrix3fv(glGetUniformLocation(shader, "rotationR"), 1, GL_FALSE, glm::value_ptr(transformR));
        glUniformMatrix3fv(glGetUniformLocation(shader, "rotationG"), 1, GL_FALSE, glm::value_ptr(transformG));
        glUniformMatrix3fv(glGetUniformLocation(shader, "rotationB"), 1, GL_FALSE, glm::value_ptr(transformB));
    }
    

    glUniform1i(glGetUniformLocation(shader, "grayScale"), (int)grayScale);
    glUniform1i(glGetUniformLocation(shader, "size"), size);
    glUniform1f(glGetUniformLocation(shader, "intensity"), intensity);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, HBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, HBuffer);
}

bool HalfToning::RenderGui()
{
    bool changed=false;
    changed |= ImGui::DragFloat("Intensity", &intensity, 0.01f);
    
    changed |= ImGui::Checkbox("GrayScale", &grayScale);

    if(grayScale)
    {
        changed |= ImGui::DragFloat("Rotation", &rotation.x, 1.0f);
    }
    else
    {
        changed |= ImGui::DragFloat("Rotation R", &rotation.x, 1.0f);
        changed |= ImGui::DragFloat("Rotation G", &rotation.y, 1.0f);
        changed |= ImGui::DragFloat("Rotation B", &rotation.z, 1.0f);
    }
    shouldRecalculateH |= ImGui::Combo("Mask Type", &maskType, "Type0\0Type1\0\0");
    // shouldRecalculateKernel |= ImGui::SliderFloat("Sigma", &sigma, 1, 10);

    changed |= shouldRecalculateH;
    return changed;
}

void HalfToning::Unload()
{
    glDeleteBuffers(1, &HBuffer);
}
//

//
//------------------------------------------------------------------------
Dithering::Dithering(bool enabled) : ImageProcess("Dithering", "", enabled)
{  
    addNoise.density=2;
    addNoise.intensity=0.05f;
    threshold.binary=true;
    threshold.global=true;
    threshold.globalLower = 0.3f;
    threshold.globalUpper = 1;
}

void Dithering::SetUniforms()
{
}

bool Dithering::RenderGui()
{
    bool changed=false;
    changed |= addNoise.RenderGui();
    changed |= threshold.RenderGui();
    return changed;
}
void Dithering::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    if(tmpTexture.width != width || tmpTexture.height != height || !tmpTexture.loaded)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.srgb=true;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        tmpTexture = GL_TextureFloat(width, height, tci);         
    }

    addNoise.Process(textureIn, tmpTexture.glTex, width, height);
    threshold.Process(tmpTexture.glTex, textureOut, width, height);
}
void Dithering::Unload()
{

}
//

//
//------------------------------------------------------------------------
Erosion::Erosion(bool enabled) : ImageProcess("Erosion", "shaders/Erosion.glsl", enabled)
{
    kernel.resize(maxSize * maxSize);
    glGenBuffers(1, (GLuint*)&kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     
}

void Erosion::RecalculateKernel()
{
    int offset = (size - subSize) / 2;
    float halfSize = (size-1)/2.0f;
    // normalising the Kernel
    std::fill(kernel.begin(), kernel.begin() + size*size, 0.0f);
    if(shape==StructuringElementShape::Circle)
    {
        for (int i = 0; i < size; ++i)
        {
            for (int j = 0; j < size; ++j)
            {
                glm::vec2 coord((float)i - halfSize, (float)j-halfSize);
                float length = glm::length(coord);
                int flatInx = (i) * size + (j);
                if(length <= halfSize)
                {
                    kernel[flatInx] = 1;
                }
            }
        }
    }
    else if(shape == StructuringElementShape::Diamond)
    {
        glm::mat3 transform(1);
        float cosTheta = cos(glm::radians(45.0f));
        float sinTheta = sin(glm::radians(45.0f));
        glm::mat3 rotationMatrix(1);
        rotationMatrix[0][0] = cosTheta;
        rotationMatrix[1][0] = -sinTheta;
        rotationMatrix[0][1] = sinTheta;
        rotationMatrix[1][1] = cosTheta;

        for (int i = 0; i < size; ++i)
        {
            for (int j = 0; j < size; ++j)
            {
                glm::vec2 coord((float)i - halfSize, (float)j-halfSize);
                coord = rotationMatrix * glm::vec3(coord, 1);
                coord += halfSize;
                int flatInx = ((int)coord.x) * size + ((int)coord.y);
                kernel[flatInx] = 1;
            }
        }        
    }
    else if(shape == StructuringElementShape::Line)
    {
        glm::mat3 transform(1);
        float cosTheta = cos(glm::radians(rotation));
        float sinTheta = sin(glm::radians(rotation));
        glm::mat3 rotationMatrix(1);
        rotationMatrix[0][0] = cosTheta;
        rotationMatrix[1][0] = -sinTheta;
        rotationMatrix[0][1] = sinTheta;
        rotationMatrix[1][1] = cosTheta;

        for(int i=0; i<size; i++)
        {
            glm::vec2 coord((float)i - halfSize, 0);
            coord = rotationMatrix * glm::vec3(coord, 1);
            coord += halfSize;
            int flatInx = ((int)coord.x) * size + ((int)coord.y);
            kernel[flatInx] = 1;            
        }
    }
    else if(shape == StructuringElementShape::Square)
    {
        for(int i=0; i<subSize; i++)
        {
            for(int j=0; j<subSize; j++)
            {
                glm::ivec2 coord = glm::ivec2(i, j) + glm::ivec2(offset);
                int flatInx = ((int)coord.x) * size + ((int)coord.y);
                kernel[flatInx] = 1;            
            }
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size * size * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    shouldRecalculateKernel=false;
}

void Erosion::SetUniforms()
{
    if(shouldRecalculateKernel) RecalculateKernel();

    glUniform1i(glGetUniformLocation(shader, "size"), size);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
}

bool Erosion::RenderGui()
{
    bool changed=false;
    shouldRecalculateKernel |= ImGui::DragInt("Size", &size, 2, 1, maxSize);
    
    shouldRecalculateKernel |= ImGui::Combo("Shape", (int*)&shape, "Circle\0Diamond\0Line\0Octagon\0Square\0\0");

    if(shape == StructuringElementShape::Line)
    {
        shouldRecalculateKernel |= ImGui::SliderFloat("Rotation", &rotation, -180, 180);
    }
    if(shape == StructuringElementShape::Square)
    {
        shouldRecalculateKernel |= ImGui::DragInt("Square Size", &subSize, 1, 1, size);
    }

    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            int flatInx = (i) * size + (j);
            ImGui::PushID(flatInx);
            ImGui::SetNextItemWidth(20);
            ImGui::DragFloat("", &kernel[flatInx]);
            ImGui::PopID();
            if(j < size-1) ImGui::SameLine();
        }
    }
    

    changed |= shouldRecalculateKernel;
    return changed;
}

void Erosion::Unload()
{
    glDeleteBuffers(1, &kernelBuffer);
}
//

//
//------------------------------------------------------------------------
AddGradient::AddGradient(bool enabled) : ImageProcess("AddGradient", "shaders/AddGradient.glsl", enabled)
{
    TextureCreateInfo tci = {};
    tci.generateMipmaps =false;
    tci.srgb=true;
    tci.minFilter = GL_LINEAR;
    tci.magFilter = GL_LINEAR;  
    tci.wrapS = GL_MIRRORED_REPEAT;      
    tci.wrapT = GL_MIRRORED_REPEAT;      
    gradientTexture = GL_TextureFloat(255, 1, tci);  

    gradientData.resize(255);     

    colorStops = 
    {
        {glm::vec3(1,0,0), 0},
        {glm::vec3(0,1,0), 0.5},
        {glm::vec3(0,0,1), 1}
    };

    RecalculateKernel();
}

void AddGradient::RecalculateKernel()
{
    std::sort(colorStops.begin(), colorStops.end(), [](const ColorStop& a, const ColorStop &b) -> bool 
    {
        return a.t < b.t;
    });        

    int runningInx=0;
    for(int i=0; i<colorStops.size()-1; i++)
    {
        float length = colorStops[i+1].t - colorStops[i].t;
        int intLength = (int)(length * 255);

        glm::vec3 prevColor = colorStops[i].color;
        glm::vec3 nextColor = colorStops[i+1].color;

        for(int j=0; j<intLength; j++)
        {
            float t = (float)j / (float)intLength;
            glm::vec3 color = glm::lerp(prevColor, nextColor, t);
            gradientData[runningInx + j] = glm::vec4(color  ,1);
        }
        runningInx += intLength;
    }
    

    glBindTexture(GL_TEXTURE_2D, gradientTexture.glTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 255, 1, 0, GL_RGBA, GL_FLOAT, gradientData.data());
    glBindTexture(GL_TEXTURE_2D, 0);    
}

void AddGradient::SetUniforms()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gradientTexture.glTex);
    glUniform1i(glGetUniformLocation(shader, "gradientTexture"), 0);
    
    glUniformMatrix2fv(glGetUniformLocation(shader, "transform"), 1, GL_FALSE, glm::value_ptr(transformMatrix));
}

bool AddGradient::RenderGui()
{
    bool changed=false;

    bool rotationChanged = ImGui::DragFloat("Rotation", &rotation, 1);
    if(rotationChanged)
    {
        changed=true;
        float cosTheta = cos(glm::radians(rotation));
        float sinTheta = sin(glm::radians(rotation));
        transformMatrix[0][0] = cosTheta;
        transformMatrix[1][0] = -sinTheta;
        transformMatrix[0][1] = sinTheta;
        transformMatrix[1][1] = cosTheta;
        transformMatrix = glm::inverse(transformMatrix);    
    }

    if(ImGui::Button("Add")){
        changed=true;
        colorStops.push_back({
            glm::vec3(1),0
        });
    }

    std::vector<int> toRemove;
    float width = ImGui::GetContentRegionMax().x;
    int maxItems = 3;
    float itemWidth = width / maxItems;
    for(int i=0; i<colorStops.size(); i++)
    {
        ImGui::SetNextItemWidth(itemWidth);
        ImGui::PushID(i * 2 + 0);
        changed |= ImGui::ColorEdit3("Color", glm::value_ptr(colorStops[i].color));
        ImGui::PopID();
        
        if(i!= 0 && i != colorStops.size()-1)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::PushID(i * 2 + 0);
            changed |= ImGui::SliderFloat("t", &colorStops[i].t, 0, 1);
            ImGui::PopID();

            ImGui::SameLine();
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::PushID(i * 2 + 0);
            if(ImGui::Button("x"))
            {
                toRemove.push_back(i);
                changed = true;
            }
            ImGui::PopID();            
        }
        
        ImGui::Separator();
    }

    for(int i=(int)toRemove.size()-1; i>=0; i--)
    {
        colorStops.erase(colorStops.begin() + toRemove[i]);
    }

    if(changed)
    {
        RecalculateKernel();
    }

    return true;
}

void AddGradient::Unload()
{
    gradientTexture.Unload();
}
//

//
//------------------------------------------------------------------------
Dilation::Dilation(bool enabled) : ImageProcess("Dilation", "shaders/Dilation.glsl", enabled)
{
    kernel.resize(maxSize * maxSize);
    glGenBuffers(1, (GLuint*)&kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     
}

void Dilation::RecalculateKernel()
{
    int offset = (size - subSize) / 2;
    float halfSize = (size-1)/2.0f;
    // normalising the Kernel
    std::fill(kernel.begin(), kernel.begin() + size*size, 0.0f);
    if(shape==StructuringElementShape::Circle)
    {
        for (int i = 0; i < size; ++i)
        {
            for (int j = 0; j < size; ++j)
            {
                glm::vec2 coord((float)i - halfSize, (float)j-halfSize);
                float length = glm::length(coord);
                int flatInx = (i) * size + (j);
                if(length <= halfSize)
                {
                    kernel[flatInx] = 1;
                }
            }
        }
    }
    else if(shape == StructuringElementShape::Diamond)
    {
        glm::mat3 transform(1);
        float cosTheta = cos(glm::radians(45.0f));
        float sinTheta = sin(glm::radians(45.0f));
        glm::mat3 rotationMatrix(1);
        rotationMatrix[0][0] = cosTheta;
        rotationMatrix[1][0] = -sinTheta;
        rotationMatrix[0][1] = sinTheta;
        rotationMatrix[1][1] = cosTheta;

        for (int i = 0; i < size; ++i)
        {
            for (int j = 0; j < size; ++j)
            {
                glm::vec2 coord((float)i - halfSize, (float)j-halfSize);
                coord = rotationMatrix * glm::vec3(coord, 1);
                coord += halfSize;
                int flatInx = ((int)coord.x) * size + ((int)coord.y);
                kernel[flatInx] = 1;
            }
        }        
    }
    else if(shape == StructuringElementShape::Line)
    {
        glm::mat3 transform(1);
        float cosTheta = cos(glm::radians(rotation));
        float sinTheta = sin(glm::radians(rotation));
        glm::mat3 rotationMatrix(1);
        rotationMatrix[0][0] = cosTheta;
        rotationMatrix[1][0] = -sinTheta;
        rotationMatrix[0][1] = sinTheta;
        rotationMatrix[1][1] = cosTheta;

        for(int i=0; i<size; i++)
        {
            glm::vec2 coord((float)i - halfSize, 0);
            coord = rotationMatrix * glm::vec3(coord, 1);
            coord += halfSize;
            int flatInx = ((int)coord.x) * size + ((int)coord.y);
            kernel[flatInx] = 1;            
        }
    }
    else if(shape == StructuringElementShape::Square)
    {
        for(int i=0; i<subSize; i++)
        {
            for(int j=0; j<subSize; j++)
            {
                glm::ivec2 coord = glm::ivec2(i, j) + glm::ivec2(offset);
                int flatInx = ((int)coord.x) * size + ((int)coord.y);
                kernel[flatInx] = 1;            
            }
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size * size * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    shouldRecalculateKernel=false;
}

void Dilation::SetUniforms()
{
    if(shouldRecalculateKernel) RecalculateKernel();

    glUniform1i(glGetUniformLocation(shader, "size"), size);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
}

bool Dilation::RenderGui()
{
    bool changed=false;
    shouldRecalculateKernel |= ImGui::DragInt("Size", &size, 2, 1, maxSize);
    
    shouldRecalculateKernel |= ImGui::Combo("Shape", (int*)&shape, "Circle\0Diamond\0Line\0Octagon\0Square\0\0");

    if(shape == StructuringElementShape::Line)
    {
        shouldRecalculateKernel |= ImGui::SliderFloat("Rotation", &rotation, -180, 180);
    }
    if(shape == StructuringElementShape::Square)
    {
        shouldRecalculateKernel |= ImGui::DragInt("Square Size", &subSize, 1, 1, size);
    }

    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            int flatInx = (i) * size + (j);
            ImGui::PushID(flatInx);
            ImGui::SetNextItemWidth(20);
            ImGui::DragFloat("", &kernel[flatInx]);
            ImGui::PopID();
            if(j < size-1) ImGui::SameLine();
        }
    }
    

    changed |= shouldRecalculateKernel;
    return changed;
}

void Dilation::Unload()
{
    glDeleteBuffers(1, &kernelBuffer);
}
//

//
//------------------------------------------------------------------------
AddImage::AddImage(bool enabled, char newFileName[128]) : ImageProcess("AddImage", "shaders/AddImage.glsl", enabled)
{
    strcpy(this->fileName, newFileName);
    if(strcmp(this->fileName, "")!=0) 
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
        aspectRatio = (float)texture.width / (float)texture.height;
		filenameChanged = false;        
    }
}


void AddImage::SetUniforms()
{
    
    glUniform1f(glGetUniformLocation(shader, "multiplier"), multiplier);    
    glUniform1f(glGetUniformLocation(shader, "aspectRatio"), aspectRatio);    


    glUniform1i(glGetUniformLocation(shader, "textureToAdd"), 2); //program must be active
    glBindImageTexture(2, texture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    
    
}

bool AddImage::RenderGui()
{
    bool changed=false;
    changed |= ImGui::DragFloat("multiplier", &multiplier, 0.001f);
    
    filenameChanged |= ImGui::InputText("File Name", fileName, IM_ARRAYSIZE(fileName));
    
    if(filenameChanged)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
        
		changed=true;
		filenameChanged = false;
    }

    if(ImGui::Button("Set Resolution from this image"))
    {
        imageProcessStack->Resize(texture.width, texture.height);
        changed=true;
    }

    return changed;
}

void AddImage::Unload()
{
    texture.Unload();
}

void AddImage::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    if(texture.loaded)
    {
        glm::ivec2 offset = glm::ivec2(width, height) - glm::ivec2(texture.width, texture.height);
        offset /=2;
        glUseProgram(shader);
        SetUniforms();

        glUniform2iv(glGetUniformLocation(shader, "offset"), 1, glm::value_ptr(offset)); //program must be active
        
        glUniform1i(glGetUniformLocation(shader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

        glUniform1i(glGetUniformLocation(shader, "textureOut"), 1); //program must be active
        glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
}
//

//
//------------------------------------------------------------------------
MultiplyImage::MultiplyImage(bool enabled, char newFileName[128]) : ImageProcess("MultiplyImage", "shaders/MultiplyImage.glsl", enabled)
{
    strcpy(this->fileName, newFileName);
    if(strcmp(this->fileName, "")!=0) filenameChanged=true;
}


void MultiplyImage::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "multiplier"), multiplier);    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.glTex);
    glUniform1i(glGetUniformLocation(shader, "textureToMultiply"), 0);
}

bool MultiplyImage::RenderGui()
{
    bool changed=false;
    changed |= ImGui::DragFloat("multiplier", &multiplier, 0.001f);
    
    filenameChanged |= ImGui::InputText("File Name", fileName, IM_ARRAYSIZE(fileName));
    
    if(filenameChanged)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
        
		changed=true;
		filenameChanged = false;
    }

    return changed;
}

void MultiplyImage::Unload()
{
    texture.Unload();
}
//

//
//------------------------------------------------------------------------
GaussianPyramid::GaussianPyramid(bool enabled) : ImageProcess("GaussianPyramid", "shaders/CopyTexture.glsl", enabled)
{
    gaussianBlur.size=5;
    gaussianBlur.sigma=5;

    pyramid.resize(depth);
    pyramidTmp.resize(depth);
}


void GaussianPyramid::SetUniforms()
{
}

bool GaussianPyramid::RenderGui()
{
    bool changed=false;
    depthChanged = ImGui::SliderInt("Depth", &depth, 0, 10);
    
    if(depthChanged)
    {
        pyramid.resize(depth);
        pyramidTmp.resize(depth);
    }

    changed |= ImGui::SliderInt("Output", &output, 0, (int)depth-1);

    changed |= depthChanged;
    changed |= gaussianBlur.RenderGui();
    return changed;
}

void GaussianPyramid::Unload()
{
  
}

void GaussianPyramid::CopyTexture(GLuint textureIn, GLuint textureOut, int width, int height)
{
 	glUseProgram(shader);
	
    //Bind sampler
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glUniform1i(glGetUniformLocation(shader, "gradientTexture"), 0);
    
    //Bind image
    glUniform1i(glGetUniformLocation(shader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
     
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);   
}

void GaussianPyramid::Build(GLuint textureIn, int width, int height)
{
    if(currentWidth != width || currentHeight != height || depthChanged)
    {
        currentWidth = width;
        currentHeight = height;

        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR; 

        int levelWidth = width;
        int levelHeight = height;
        for(int i=0; i<pyramid.size(); i++)
        {
            if(pyramid[i].loaded) pyramid[i].Unload();
            if(pyramidTmp[i].loaded) pyramidTmp[i].Unload();
            pyramid[i] = GL_TextureFloat(levelWidth, levelHeight, tci);
            pyramidTmp[i] = GL_TextureFloat(levelWidth, levelHeight, tci);
            levelWidth/=2;
            levelHeight/=2;
        }
    }

  //Copy TextureIn into pyramid[0];
    CopyTexture(textureIn, pyramid[0].glTex, pyramid[0].width, pyramid[0].height);
    for(int i=0; i<pyramid.size()-1; i++)
    {
#if 1
        gaussianBlur.Process(pyramid[i].glTex, pyramidTmp[i].glTex, pyramid[i].width, pyramid[i].height);
        CopyTexture(pyramidTmp[i].glTex, pyramid[i+1].glTex, pyramid[i+1].width, pyramid[i+1].height);
#else
        CopyTexture(pyramid[i].glTex, pyramid[i+1].glTex, pyramid[i+1].width, pyramid[i+1].height);
#endif
    }
}

void GaussianPyramid::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    Build(textureIn, width, height);
    CopyTexture(pyramid[output].glTex, textureOut, width, height);
}
//

//
//------------------------------------------------------------------------
LaplacianPyramid::LaplacianPyramid(bool enabled) : ImageProcess("LaplacianPyramid", "shaders/SubtractTexture.glsl", enabled)
{
    CreateComputeShader("shaders/AddTexture.glsl", &addTextureShader);
    CreateComputeShader("shaders/ClearTexture.glsl", &clearTextureShader);
}


void LaplacianPyramid::SetUniforms()
{
}

bool LaplacianPyramid::RenderGui()
{
    bool changed=false;

    changed |= gaussianPyramid.RenderGui();

    ImGui::Separator();
    changed |= ImGui::SliderInt("Laplacian Output", &output, 0, gaussianPyramid.depth-1);
    changed |= ImGui::Checkbox("Output Reconstruction", &outputReconstruction);
    return changed;
}

void LaplacianPyramid::Unload()
{
  
}


void LaplacianPyramid::SubtractTexture(GLuint textureA, GLuint textureB, int width, int height)
{
 	glUseProgram(shader);
	
    //Bind image A
    glUniform1i(glGetUniformLocation(shader, "textureA"), 0); //program must be active
    glBindImageTexture(0, textureA, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
     
    //Bind sampler
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glUniform1i(glGetUniformLocation(shader, "textureB"), 0);
    
     
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);   
}

void LaplacianPyramid::AddTexture(GLuint textureA, GLuint textureB, int width, int height)
{
 	glUseProgram(addTextureShader);
	
    //Bind image A
    glUniform1i(glGetUniformLocation(addTextureShader, "textureA"), 0); //program must be active
    glBindImageTexture(0, textureA, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
     
    //Bind sampler
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glUniform1i(glGetUniformLocation(addTextureShader, "textureB"), 0);
    
     
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);   
}

void LaplacianPyramid::ClearTexture(GLuint texture, int width, int height)
{
    glUseProgram(clearTextureShader);
    glUniform1i(glGetUniformLocation(clearTextureShader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(width/32+1, height/32+1, 1);
    glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);     
}

void LaplacianPyramid::Build(GLuint textureIn, int width, int height)
{
    gaussianPyramid.Build(textureIn, width, height);

    for(int i=0; i<gaussianPyramid.pyramid.size()-1; i++)
    {
        SubtractTexture(gaussianPyramid.pyramid[i].glTex, 
                        gaussianPyramid.pyramid[i+1].glTex, 
                        gaussianPyramid.pyramid[i].width, 
                        gaussianPyramid.pyramid[i].height);
    }
        
}


void LaplacianPyramid::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    Build(textureIn, width, height);

    if(outputReconstruction)
    {
        ClearTexture(textureOut, width, height);
        for(int i=gaussianPyramid.depth-1; i>=0; i--)
        {

            AddTexture(textureOut, gaussianPyramid.pyramid[i].glTex, width, height);
        }
    }
    else
    {
        gaussianPyramid.CopyTexture(gaussianPyramid.pyramid[output].glTex, textureOut, width, height);
    }


}
//

//
//------------------------------------------------------------------------
PenDraw::PenDraw(bool enabled) : ImageProcess("PenDraw", "shaders/AddImage.glsl", enabled)
{
}
	

void PenDraw::SetUniforms()
{
    if(paintTexture.width != imageProcessStack->width || paintTexture.height != imageProcessStack->height || !paintTexture.loaded)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR; 
        if(paintTexture.loaded) paintTexture.Unload();
        paintTexture = GL_TextureFloat(imageProcessStack->width, imageProcessStack->height, tci);

        paintData.resize(paintTexture.width * paintTexture.height);
        paintedPixels.resize(paintTexture.width * paintTexture.height, false);
    }

    glUniform1f(glGetUniformLocation(shader, "multiplier"), 1);    

    glUniform1i(glGetUniformLocation(shader, "textureToAdd"), 2); //program must be active
    glBindImageTexture(2, paintTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
}

bool PenDraw::RenderGui()
{
    bool changed=false;
    selected=true;
    
    if(ImGui::Button("Clear"))
    {
        std::fill(paintData.begin(), paintData.end(), glm::vec4(0,0,0,1));
        std::fill(paintedPixels.begin(), paintedPixels.end(), false);
        glBindTexture(GL_TEXTURE_2D, paintTexture.glTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, paintTexture.width, paintTexture.height, 0, GL_RGBA, GL_FLOAT, paintData.data());
        glBindTexture(GL_TEXTURE_2D, 0);              
        changed=true;
    }

    changed |= ImGui::SliderInt("Radius", &radius, 1, 25);
    changed |= ImGui::ColorEdit3("Color", glm::value_ptr(color));

    return changed;
}


void PenDraw::Unload()
{
    paintTexture.Unload();
}

bool PenDraw::MouseMove(float x, float y) 
{
    if(x<0 || !selected) return false;

    bool drawChanged=false;
    glm::vec2 currentMousPos(x, y);
    
    if(mousePressed && previousMousPos != currentMousPos)
    {
        glm::vec2 diff = currentMousPos - previousMousPos;
        float diffLength = std::ceil(glm::length(diff));
        for(float i=0; i<diffLength; i++)
        {
            float t = i / diffLength;
            glm::vec2 delta = t * diff;
            glm::ivec2 c = previousMousPos + delta;
            
#if 0
            if(contourPoints.size()==0 || (contourPoints.size() > 0 && c != contourPoints[contourPoints.size()-1]))
            {
                contourPoints.push_back(c);
                paintedPixels[c.y * paintTexture.width + c.x]=true;
                paintData[c.y * paintTexture.width + c.x ] = glm::vec4(color,1);
            }
#endif

            for(int ky=-radius; ky <=radius; ky++)
            {
                for(int kx=-radius; kx <=radius; kx++)
                {
                    if((ky*ky + kx+kx) < radius*radius)
                    {
                        glm::ivec2 coord(c.x + kx, c.y + ky);
                        int inx = coord.y * paintTexture.width + coord.x;
                        paintData[inx] = glm::vec4(color,1);
                    }
                }
            }
        }


        glBindTexture(GL_TEXTURE_2D, paintTexture.glTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, paintTexture.width, paintTexture.height, 0, GL_RGBA, GL_FLOAT, paintData.data());
        glBindTexture(GL_TEXTURE_2D, 0);        

        drawChanged=true;
    }

    previousMousPos = currentMousPos;

    selected=false;

    return drawChanged;
}

bool PenDraw::MousePressed() 
{
    mousePressed=true;
    contourPoints.clear();
    return false;
}

bool PenDraw::MouseReleased() 
{
    mousePressed=false;
    return false;
}


//

//
//------------------------------------------------------------------------
HardComposite::HardComposite(bool enabled, char* newFileName) : ImageProcess("HardComposite", "", enabled)
{
    CreateComputeShader("shaders/HardCompositeViewMask.glsl", &viewMaskShader);
    CreateComputeShader("shaders/HardComposite.glsl", &compositeShader);
    
    strcpy(this->fileName, newFileName);
    if(strcmp(this->fileName, "")!=0) 
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
    	filenameChanged = false;        
    }    
}


void HardComposite::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    if(drawingMask)
    {
        glUseProgram(viewMaskShader);
        SetUniforms();

        glUniform1i(glGetUniformLocation(viewMaskShader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "textureOut"), 1); //program must be active
        glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "mask"), 2); //program must be active
        glBindImageTexture(2, maskTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "sourceTexture"), 3); //program must be active
        glBindImageTexture(3, texture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    else
    {
        if(doBlur)
        {
            smoothFilter.Process(maskTexture.glTex, smoothedMaskTexture.glTex, maskTexture.width, maskTexture.height);
        }

        glUseProgram(compositeShader);
        SetUniforms();

        glUniform1i(glGetUniformLocation(compositeShader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(compositeShader, "textureOut"), 1); //program must be active
        glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(compositeShader, "mask"), 2); //program must be active
        glBindImageTexture(2, doBlur ? smoothedMaskTexture.glTex : maskTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(compositeShader, "sourceTexture"), 3); //program must be active
        glBindImageTexture(3, texture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
}

void HardComposite::SetUniforms()
{
    if(maskTexture.width != imageProcessStack->width || maskTexture.height != imageProcessStack->height || !maskTexture.loaded)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR; 
        if(maskTexture.loaded) maskTexture.Unload();
        if(smoothedMaskTexture.loaded) smoothedMaskTexture.Unload();
        maskTexture = GL_TextureFloat(imageProcessStack->width, imageProcessStack->height, tci);
        smoothedMaskTexture = GL_TextureFloat(imageProcessStack->width, imageProcessStack->height, tci);

        maskData.resize(maskTexture.width * maskTexture.height);
    }

    glUniform1i(glGetUniformLocation(shader, "mask"), 2); //program must be active
    glBindImageTexture(2, maskTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
}

bool HardComposite::RenderGui()
{
    bool changed=false;
    selected=true;
    
    ImGui::Text("Mask");
    changed |= ImGui::Checkbox("Draw Mask", &drawingMask);
    if(drawingMask)
    {
        changed |= ImGui::SliderInt("Radius", &radius, 1, 25);
        changed |= ImGui::Checkbox("Add", &adding);
    }

    ImGui::Separator();
    filenameChanged |= ImGui::InputText("File Name", fileName, IM_ARRAYSIZE(fileName));
    if(filenameChanged)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
        
		changed=true;
		filenameChanged = false;
    }

    changed |= ImGui::Checkbox("Weighted Transition", &doBlur);
    if(doBlur)
    {
        changed |= ImGui::DragInt("Transition Radius", &smoothFilter.size, 1, 0, 10000);
    }

    return changed;
}


void HardComposite::Unload()
{
    maskTexture.Unload();
    texture.Unload();
    glDeleteProgram(viewMaskShader);
}

bool HardComposite::MouseMove(float x, float y) 
{
    if(x<0 || !selected && !drawingMask) return false;

    bool drawChanged=false;
    glm::vec2 currentMousPos(x, y);
    
    if(mousePressed && previousMousPos != currentMousPos)
    {
        glm::vec2 diff = currentMousPos - previousMousPos;
        float diffLength = std::ceil(glm::length(diff));
        for(float i=0; i<diffLength; i++)
        {
            float t = i / diffLength;
            glm::vec2 delta = t * diff;
            glm::ivec2 c = previousMousPos + delta;
            for(int ky=-radius; ky <=radius; ky++)
            {
                for(int kx=-radius; kx <=radius; kx++)
                {
                    if((ky*ky + kx+kx) < radius*radius)
                    {
                        glm::ivec2 coord(c.x + kx, c.y + ky);
                        if(coord.x <0 || coord.y < 0 || coord.x >= maskTexture.width || coord.y >= maskTexture.height) continue;
                        int inx = coord.y * maskTexture.width + coord.x;
                        maskData[inx] = adding ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1);
                    }
                }
            }
        }


        glBindTexture(GL_TEXTURE_2D, maskTexture.glTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, maskTexture.width, maskTexture.height, 0, GL_RGBA, GL_FLOAT, maskData.data());
        glBindTexture(GL_TEXTURE_2D, 0);        

        drawChanged=true;
    }

    previousMousPos = currentMousPos;

    selected=false;

    return drawChanged;
}

bool HardComposite::MousePressed() 
{
    mousePressed=true;
    return false;
}

bool HardComposite::MouseReleased() 
{
    mousePressed=false;
    return false;
}

//
//------------------------------------------------------------------------
SeamCarvingResize::SeamCarvingResize(bool enabled) : ImageProcess("SeamCarvingResize", "", enabled)
{    
}

float SeamCarvingResize::CalculateCostAt(glm::ivec2 position, std::vector<glm::vec4> &image, int width, int height)
{
    int inx = position.y * width + position.x;
    glm::vec4 nextX = (position.x >=0 && position.x < width-2) ?  image[inx+1] : glm::vec4(1e30f);
    glm::vec4 nextY = (position.y >=0 && position.y < height-2) ?  image[inx + width] : glm::vec4(0);
    
    float currentGrayScale = Color2GrayScale(image[inx]);
    float nextXGrayScale  =  Color2GrayScale(nextX);
    float nextYGrayScale  =  Color2GrayScale(nextY);

    float gradX = nextXGrayScale - currentGrayScale;
    float gradY = nextYGrayScale - currentGrayScale;
    

    return (gradX * gradX + gradY * gradY);
}

void SeamCarvingResize::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    //Read back gradient
    if(width != downSizeSeamsDone.size()) 
    {
        downSizeSeamsDone.resize(width);
        downSizeSeams.clear();
        upSizeSeamsDone.resize(width);
        upSizeSeams.clear();
        imageData.resize(width * height, glm::vec4(0));
        iterations=0;
    }

    if(iterations==0)
    {
		upSizeSeamsDone.resize(width);
		upSizeSeams.clear();
		imageData.resize(width * height, glm::vec4(0));
		originalData.resize(width * height, glm::vec4(0));
        resizedWidth = width;

        glBindTexture(GL_TEXTURE_2D, textureIn);
        glGetTexImage (GL_TEXTURE_2D,
                        0,
                        GL_RGBA, // GL will convert to this format
                        GL_FLOAT,   // Using this data type per-pixel
                        originalData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        imageData = originalData;
    }

    if(debug)
    {
        debugData.resize(width * height);
        glBindTexture(GL_TEXTURE_2D, textureIn);
        glGetTexImage (GL_TEXTURE_2D,
                        0,
                        GL_RGBA, // GL will convert to this format
                        GL_FLOAT,   // Using this data type per-pixel
                        debugData.data());
        glBindTexture(GL_TEXTURE_2D, 0);        
    }

    std::vector<float> costs(width*height, 0);
    std::vector<int> directions(width*height, 0);
    for(int k=0; k<numPerIterations; k++)
    {
        if(increase)
        {
            //Calculate costs on top line
            for(int x=0; x<resizedWidth; x++)
            {
                int y=0;
                int inx = y * width + x; 
                costs[inx] = CalculateCostAt(glm::ivec2(x, y), imageData, width, height);
                if(upSizeSeamsDone[x])
                {
                    costs[inx] = 1e30f;
                }
            }

            //calculate cost at each pixel
            for(int y=1; y<height; y++)
            {
                for(int x=0; x<resizedWidth; x++)
                {
                    int inx = y * width + x; 
                    costs[inx] = CalculateCostAt(glm::ivec2(x, y), imageData, width, height);
                    
                    float topCost = costs[inx-width];
                    float topLeftCost = costs[inx-width - 1];
                    float topRightCost = costs[inx-width + 1];
                    if(x==0) topLeftCost=1e30f;
                    if(x==width-2) topRightCost=1e30f;


                    if(topCost < topLeftCost && topCost < topRightCost)
                    {
                        costs[inx] += topCost;
                        directions[inx] = 0;
                    }
                    else if(topLeftCost < topCost && topLeftCost < topRightCost)
                    {
                        costs[inx] += topLeftCost;
                        directions[inx] = -1;
                    }
                    else if(topRightCost < topCost && topRightCost < topLeftCost)
                    {
                        costs[inx] += topRightCost;
                        directions[inx] = 1;
                    }
                }
            }

            //Find the lowest cost on bottom line
            glm::ivec2 lowestInx(0);
            float lowestCost = 1e30f;
            for(int x=0; x<resizedWidth; x++)
            {
                int y = height-1;
                int inx = y * width + x;
                float cost = costs[inx];
                if(cost < lowestCost)
                {
                    lowestCost=cost;
                    lowestInx=glm::ivec2(x, y);
                }
            }        

            //Build seam line
            upSizeSeams.resize(upSizeSeams.size()+1);
            upSizeSeams[upSizeSeams.size()-1].points.resize(height);
            int added=0;
            glm::ivec2 currentPoint =  lowestInx;
            while(true)
            {
                upSizeSeams[upSizeSeams.size()-1].points[added++] = (currentPoint);
                int direction = directions[currentPoint.y * width + currentPoint.x];
                currentPoint = glm::ivec2(currentPoint.x + direction, currentPoint.y-1);
                if(currentPoint.y==-1) 
                {
                    upSizeSeamsDone[currentPoint.x]=true;
                    break;
                }
            }


            // //Push apart the right pixels
            for(int y=0; y<height; y++)
            {
                int seamX = upSizeSeams[upSizeSeams.size()-1].points[height-y-1].x;
                
                //Set the value on the seam to be the average between the seam value and the previous pixel on the right
                //PROBLEM :
                int inx = y * width + seamX;
                glm::vec4 prev = imageData[inx-1];
                glm::vec4 next = imageData[inx+1];
                imageData[inx] = (prev + next) * 0.5f;
                
                // if(y==0)
                {
                    upSizeSeamsDone[seamX] = true;
                    // upSizeSeamsDone[inx] = true;
                    // upSizeSeamsDone[inx] = true;
                }

                //Move all the pixels at the right of the seam towards the right
                for(int x=resizedWidth; x>seamX; x--) 
                {
                    inx = y * width + x;
                    imageData[inx] = imageData[inx-1];
                }
            }
            resizedWidth++;
        }
        else
        {

            for(int x=0; x<resizedWidth; x++)
            {
                int y=0;
                int inx = y * width + x; 
                costs[inx] = CalculateCostAt(glm::ivec2(x, y), imageData, width, height);
                if(downSizeSeamsDone[x])
                {
                    costs[inx] = 1e30f;
                }
            }

            //calculate cost at each pixel
            for(int y=1; y<height; y++)
            {
                for(int x=0; x<resizedWidth; x++)
                {
                    int inx = y * width + x; 
                    costs[inx] = CalculateCostAt(glm::ivec2(x, y), imageData, width, height);
                    
                    float topCost = costs[inx-width];
                    float topLeftCost = costs[inx-width - 1];
                    float topRightCost = costs[inx-width + 1];
                    if(x==0) topLeftCost=1e30f;
                    if(x==width-2) topRightCost=1e30f;


                    if(topCost < topLeftCost && topCost < topRightCost)
                    {
                        costs[inx] += topCost;
                        directions[inx] = 0;
                    }
                    else if(topLeftCost < topCost && topLeftCost < topRightCost)
                    {
                        costs[inx] += topLeftCost;
                        directions[inx] = -1;
                    }
                    else if(topRightCost < topCost && topRightCost < topLeftCost)
                    {
                        costs[inx] += topRightCost;
                        directions[inx] = 1;
                    }
                }
            }

            //Find the lowest cost
            glm::ivec2 lowestInx(0);
            float lowestCost = 1e30f;
            for(int x=0; x<resizedWidth; x++)
            {
                int y = height-1;
                int inx = y * width + x;
                float cost = costs[inx];
                if(cost < lowestCost)
                {
                    lowestCost=cost;
                    lowestInx=glm::ivec2(x, y);
                }
            }

            downSizeSeams.resize(downSizeSeams.size()+1);
            downSizeSeams[downSizeSeams.size()-1].points.resize(height);
            int added=0;
            //Move back to top
            glm::ivec2 currentPoint =  lowestInx;
            while(true)
            {
                downSizeSeams[downSizeSeams.size()-1].points[added++] = (currentPoint);
                int direction = directions[currentPoint.y * width + currentPoint.x];
                currentPoint = glm::ivec2(currentPoint.x + direction, currentPoint.y-1);
                if(currentPoint.y==-1) 
                {
                    downSizeSeamsDone[currentPoint.x]=true;
                    break;
                }
            }

            for(int y=0; y<height; y++)
            {
                int seamX = downSizeSeams[downSizeSeams.size()-1].points[height-y-1].x;
                for(int x=seamX; x<resizedWidth; x++)
                {
                    int inx = y * width + x;
                    imageData[inx] = imageData[inx+1];
                }

                for(int x=resizedWidth; x<width; x++)
                {
                    int inx = y * width + x;
                    imageData[inx] = glm::vec4(0,0,0,1);
                }
            }
            resizedWidth--;
        }
        iterations++;
    }

    if(debug)
    {
        for(int i=0; i<downSizeSeams.size(); i++)
        {
            for(int j=0; j<downSizeSeams[i].points.size(); j++)
            {
                glm::ivec2 p = downSizeSeams[i].points[j];
                debugData[p.y * width + p.x] = glm::vec4(1,0,0,1);
            }
        }
        for(int i=0; i<upSizeSeams.size(); i++)
        {
            for(int j=0; j<upSizeSeams[i].points.size(); j++)
            {
                glm::ivec2 p = upSizeSeams[i].points[j];
                debugData[p.y * width + p.x] = glm::vec4(0,1,0,1);
            }
        }
    }    

    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, debug ? debugData.data() :  imageData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

}

void SeamCarvingResize::SetUniforms()
{
}

bool SeamCarvingResize::RenderGui()
{
    bool changed=false;
    ImGui::SliderInt("NumPerIterations", &numPerIterations, 1, 30);
    changed |= ImGui::Checkbox("Debug Seams", &debug);
    ImGui::Checkbox("Increase", &increase);
    if(ImGui::Button("Add"))
    {
        changed=true;
    }
    return changed;
}


//
//------------------------------------------------------------------------
int GetStartIndex(glm::ivec2 b, glm::ivec2 c)
{
    glm::ivec2 diff = c - b;
    if(diff == glm::ivec2(-1, 0)) return 0; //0:Left 
    if(diff == glm::ivec2(-1,-1)) return 1; //1:top left
    if(diff == glm::ivec2( 0,-1)) return 2; //2:top
    if(diff == glm::ivec2( 1,-1)) return 3; //3:top right
    if(diff == glm::ivec2( 1, 0)) return 4; //4:right
    if(diff == glm::ivec2( 1, 1)) return 5; //5:bottom right
    if(diff == glm::ivec2( 0, 1)) return 6; //6:bottom
    if(diff == glm::ivec2(-1, 1)) return 7; //7:bottom left
    return 0;
}

std::vector<glm::ivec2> PatchInpainting::CalculateContour(int width, int height)
{
	std::vector<glm::ivec2> result;

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
    std::vector<point> contourPoints;

    bool shouldBreak = false;
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            if(maskData[y * width + x].x != 0)
            {
                contourPoints.push_back(
                    {
                        glm::ivec2(x, y),
                        glm::ivec2(x-1, y)
                    }
                );
                shouldBreak = true;
                break;
            }
        }
        if (shouldBreak)break;
    }
    
	if (contourPoints.size() > 0)
	{
		//Find the ordered list of contourPoints
		point *currentPoint = &contourPoints[contourPoints.size()-1];
		point firstPoint = *currentPoint;
		do {
			glm::ivec2 prevCoord;
			int startInx = GetStartIndex(currentPoint->b, currentPoint->c);
        
			for(int i=startInx; i<startInx + directions.size(); i++)
			{
				int dirInx = i % directions.size();

				glm::ivec2 checkCoord = currentPoint->b + directions[dirInx];
				if(checkCoord.x <=0 || checkCoord.y <=0 || checkCoord.x >= width-1 || checkCoord.y >= height-1) continue;
				float checkValue = maskData[checkCoord.y * width + checkCoord.x].x;
				if(checkValue>0)
				{
					point newPoint = 
					{
						checkCoord,
						prevCoord,
					};
					contourPoints.push_back(newPoint);
					break;
				}
				prevCoord = checkCoord;
			}

			currentPoint = &contourPoints[contourPoints.size()-1];
		}
		while(currentPoint->b != firstPoint.b && currentPoint->c != firstPoint.c);

		//Remove duplicate point
		contourPoints.erase(contourPoints.begin() + contourPoints.size()-1);
    
		result.resize(contourPoints.size());
		for(int i=0; i<contourPoints.size(); i++)
		{
			result[i] = contourPoints[i].b;
		}
	}
    
    if(result.size()==0) //Return all the points in the mask
    {
        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                if(maskData[y * width + x].x>0) result.push_back(glm::ivec2(x, y));
            }
        }
    }
	return result;
}

PatchInpainting::PatchInpainting(bool enabled) : ImageProcess("PatchInpainting", "", enabled)
{
    CreateComputeShader("shaders/HardCompositeViewMask.glsl", &viewMaskShader); 
}

void PatchInpainting::ExtractPatch(std::vector<glm::vec4> &image, int imageWidth, int imageHeight, glm::ivec2 position, int size, std::vector<glm::vec4>& patch)
{
    int totalSize = (size*2+1) * (size*2+1);
    if(patch.size() != totalSize) patch.resize(totalSize);
    glm::ivec2 k;
    int i=0;
    for(k.y=-size; k.y<=size; k.y++)
    {
        for(k.x=-size; k.x<=size; k.x++)
        {                
            glm::ivec2 p = position + k;
            int inx = p.y * imageWidth + p.x;
            if(p.x >=0 && p.y >=0 && p.x < imageWidth-1 && p.y < imageHeight-1)
            {
                patch[i] = (image[inx]);
            }
            else
            {
                patch[i] = glm::vec4(10000);
            }

            i++;
        }
    }   
}

float PatchInpainting::ComparePatches(std::vector<glm::vec4> &patch1, std::vector<glm::vec4> &patch2)
{
    float distance =0;
    for(int i=0; i<patch1.size(); i++)
    {
        distance += glm::distance2(glm::vec3(patch1[i]), glm::vec3(patch2[i]));
    }
    return distance;
}

void PatchInpainting::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    
    if(maskTexture.width != imageProcessStack->width || maskTexture.height != imageProcessStack->height || !maskTexture.loaded)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR; 
        if(maskTexture.loaded) maskTexture.Unload();
        maskTexture = GL_TextureFloat(imageProcessStack->width, imageProcessStack->height, tci);

        maskData.resize(maskTexture.width * maskTexture.height);
    }
    
    if(drawingMask)
    {
        glUseProgram(viewMaskShader);
        SetUniforms();

        glUniform1i(glGetUniformLocation(viewMaskShader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "textureOut"), 1); //program must be active
        glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "mask"), 2); //program must be active
        glBindImageTexture(2, maskTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    else
    {
        
        if(iteration==0)
        {
            textureData.resize(width * height);
            glBindTexture(GL_TEXTURE_2D, textureIn);
            glGetTexImage (GL_TEXTURE_2D,
                            0,
                            GL_RGBA, // GL will convert to this format
                            GL_FLOAT,   // Using this data type per-pixel
                            textureData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
#if DEBUG_INPAINTING
        std::vector<glm::vec4> a = textureData;
#endif
        //Calculate contour        
        points = CalculateContour(width, height);
        
        int totalSize = (patchSize*2+1) * (patchSize*2+1);
        
        //Calculate the priorities of each boundary point
        if (points.size() > 0)
        {
            int maxInx=0;
            float highestPriority = 0;
            float pixelConfidence=0;
            for(int i=0; i<points.size(); i++)
            {
                //Calculate confidence
                float confidence = 0;
                glm::ivec2 k;
                for(k.y=-patchSize; k.y<=patchSize; k.y++)
                {
                    for(k.x=-patchSize; k.x<=patchSize; k.x++)
                    {
                        glm::ivec2 c = points[i] + k;
                        int inx = c.y * width + c.x;
                        confidence += maskData[inx].y; //Confidence is stored in y. starts with 1
                    }
                }
                confidence /= (float)(totalSize);

                //Calculate gradient
                float sobelKernel[9] = { -1, -2, -1, 
                                      0,  0,  0,
                                      1,  2,  1};
                float gradX=0;
                float gradY=0;
				int ky = 0;
                for(k.y=-1; k.y<=1; k.y++)
                {
					int kx = 0;
                    for(k.x=-1; k.x<=1; k.x++)
                    {
                        glm::ivec2 c = points[i] + k;
                        int inx = c.y * width + c.x;
                        
                        float weightX = sobelKernel[ky * 3 + kx];
                        float weightY = sobelKernel[kx * 3 + ky];

                        glm::vec4 pixelColor = textureData[inx];
                        float grayScale = (pixelColor.r + pixelColor.g + pixelColor.b) * 0.3333f;
                        gradX += grayScale * weightX;         
                        gradY += grayScale * weightY;    
                        
                        kx++;                    
                    }
                    ky++;
                }

                float data = glm::length(glm::vec2(gradX, gradY));
                // data=1;
#if DEBUG_INPAINTING    
                // a[points[i].y * width + points[i].x] = glm::vec4(data, data, data,1);
#endif
                // confidence=1;
                float priority = confidence * data;
                if(priority>highestPriority)
                {
                    pixelConfidence = confidence;
                    highestPriority=priority;
                    maxInx=i;
                }
            }

            patchCenter = points[maxInx];
            
            //Build the patch that we need to find elsewhere in the image
            std::vector<glm::vec4> patchToCompare;
            ExtractPatch(
                textureData,
                width, height, 
                patchCenter, patchSize,
                patchToCompare
            );

            glm::ivec2 loopStart(
                searchWholeImage ? 0 : searchWindowStart.x,
                searchWholeImage ? 0 : searchWindowStart.y
            );
            glm::ivec2 loopEnd(
                searchWholeImage ? width :  searchWindowEnd.x,
                searchWholeImage ? height : searchWindowEnd.y            
            );

            std::vector<glm::vec4> currentPatch;
            //Look for the patch in the image
            float minDifference = 1e30f;
            std::vector<glm::vec4> newPatch;
            for(int y=loopStart.y; y<loopEnd.y; y+=patchSize)
            {
                for(int x=loopStart.x; x<loopEnd.x; x+=patchSize)
                {
                    glm::ivec2 c(x, y);
                    //Skip if we're at the patch that we're looking for
                    if(c == patchCenter) continue;

                    //We  check if there is unknown pixels into the patch and discard it if yes
                    bool shouldDiscard=false;
                    for(int ky=-patchSize; ky<=patchSize; ky++)
                    {
                        for(int kx=-patchSize; kx<=patchSize; kx++)
                        {                
                            glm::ivec2 p = c + glm::ivec2(kx, ky);
                            int inx = p.y * width + p.x;
                            if(p.x >=0 && p.y >=0 && p.x < width-1 && p.y < height-1)
                            {
                                if(maskData[inx].x > 0) 
                                {
                                    shouldDiscard=true;
                                    break;
                                }
                            }
                        }
                        if(shouldDiscard) break;
                    }   
                    if(shouldDiscard) continue;

                    
                    ExtractPatch(
                        textureData,
                        width, height, 
                        c, patchSize,
                        currentPatch
                    );
                    float difference = ComparePatches(currentPatch, patchToCompare);
                    

                    if(difference < minDifference)
                    {
                        minDifference = difference;
                        newPatch = currentPatch;
                    }
                }
            }

			if (newPatch.size() > 0)
			{
				//Replace the current patch with the found patch
				int i=0;
				for(int y=-patchSize; y<=patchSize; y++)
				{
					for(int x=-patchSize; x<=patchSize; x++)
					{
						glm::ivec2 outputCoord = patchCenter + glm::ivec2(x, y);
						int outputInx = outputCoord.y * width + outputCoord.x;
						textureData[outputInx] = newPatch[i];
						// maskData[outputInx].y = pixelConfidence;
						i++;
					}
				}
				maskData[patchCenter.y * width + patchCenter.x].x =0;
				iteration++;
			}

#if DEBUG_INPAINTING
            for(int k=0; k<maskData.size(); k++)
            {
                if(maskData[k].x >0) a[k] += glm::vec4(0,0.5, 0.5, 0);
            }   
            a[patchCenter.y * width + patchCenter.x] = glm::vec4(0,1,0,1);
            glBindTexture(GL_TEXTURE_2D, textureOut);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, a.data());
            glBindTexture(GL_TEXTURE_2D, 0);
#endif
        }

#if !DEBUG_INPAINTING
        glBindTexture(GL_TEXTURE_2D, textureOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, textureData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
#endif
    } 
}

void PatchInpainting::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "mask"), 2); //program must be active
    glBindImageTexture(2, maskTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
}

bool PatchInpainting::RenderGui()
{
    bool changed=false;
    selected=true;
    
    ImGui::Text("Mask");
    ImGui::Checkbox("Draw Mask", &drawingMask);
    if(drawingMask)
    {
        changed |= ImGui::Checkbox("Add", &adding);
        changed |= ImGui::SliderInt("Radius", &radius, 1, 20);

        if(ImGui::Button("Clear"))
        {
            std::fill(maskData.begin(), maskData.end(), glm::vec4(0,0,0,1));
            changed=true;

            glBindTexture(GL_TEXTURE_2D, maskTexture.glTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, maskTexture.width, maskTexture.height, 0, GL_RGBA, GL_FLOAT, maskData.data());
            glBindTexture(GL_TEXTURE_2D, 0);            
        }
    }

    ImGui::Separator();

    // iterate=false;
    if(ImGui::Button("Iterate"))
    {
        iterate=!iterate;
        // iterate=true;
    }

    ImGui::SliderInt("Patch Size", &patchSize, 1, 10);

    ImGui::Checkbox("Search Whole Image", &searchWholeImage);
    if(!searchWholeImage)
    {
        ImGui::DragIntRange2("X", &searchWindowStart.x, &searchWindowEnd.x);
        ImGui::DragIntRange2("Y", &searchWindowStart.y, &searchWindowEnd.y);
    }

    if(iterate) changed=true;
    


    return changed;
}

bool PatchInpainting::RenderOutputGui()
{
    bool changed=false;
    // float width = ImGui::GetWindowContentRegionWidth();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    glm::vec2 margin = imageProcessStack->outputGuiStart;
    
    glm::vec2 pos = (searchWindowEnd + searchWindowStart) / 2;
    glm::vec2 size = (searchWindowEnd - searchWindowStart);

    pos *= imageProcessStack->zoomLevel;

    glm::vec2 newStart = pos - size*0.5f;
    glm::vec2 newEnd =   pos + size*0.5f;

    glm::vec2 start = glm::vec2(newStart.x + margin.x, newStart.y + margin.y) * imageProcessStack->zoomLevel;
    glm::vec2 end = glm::vec2(newEnd.x   + margin.x, newEnd.y   + margin.y) * imageProcessStack->zoomLevel;
    
    drawList->AddRect(
        ImVec2(start.x, start.y), 
        ImVec2(end.x, end.y),
        IM_COL32(0,0,255,255)
    );

    return changed;    
}

void PatchInpainting::Unload()
{
    maskTexture.Unload();
    glDeleteProgram(viewMaskShader);
}

bool PatchInpainting::MouseMove(float x, float y) 
{
    if(x<0 || !selected || !drawingMask) return false;

    
    bool drawChanged=false;
    glm::vec2 currentMousPos(x, y);
    
    if(mousePressed && previousMousPos != currentMousPos)
    {
        glm::vec2 diff = currentMousPos - previousMousPos;
        float diffLength = std::ceil(glm::length(diff));
        for(float i=0; i<diffLength; i++)
        {
            float t = i / diffLength;
            glm::vec2 delta = t * diff;
            glm::ivec2 c = previousMousPos + delta;

            for(int ky=-radius; ky <=radius; ky++)
            {
                for(int kx=-radius; kx <=radius; kx++)
                {
                    if((ky*ky + kx+kx) < radius*radius)
                    {
                        glm::ivec2 coord(c.x + kx, c.y + ky);
                        if(coord.x <0 || coord.y < 0 || coord.x >= maskTexture.width || coord.y >= maskTexture.height) continue;
                        int inx = coord.y * maskTexture.width + coord.x;
                        maskData[inx] = adding ? glm::vec4(1,1,0,0) : glm::vec4(0,0,0,1);
                    }
                }
            }
        }

        glBindTexture(GL_TEXTURE_2D, maskTexture.glTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, maskTexture.width, maskTexture.height, 0, GL_RGBA, GL_FLOAT, maskData.data());
        glBindTexture(GL_TEXTURE_2D, 0);        

        drawChanged=true;
    }

    previousMousPos = currentMousPos;

    selected=false;

    return drawChanged;
}

bool PatchInpainting::MousePressed() 
{
    mousePressed=true;
    return false;
}

bool PatchInpainting::MouseReleased() 
{
    mousePressed=false;
    return false;
}


//
//
//------------------------------------------------------------------------
MultiResComposite::MultiResComposite(bool enabled, char* newFileName) : ImageProcess("MultiResComposite", "shaders/MultiResComposite.glsl", enabled)
{
    CreateComputeShader("shaders/HardCompositeViewMask.glsl", &viewMaskShader);
    
    float sigma = 5;
    int size = 9;
    
    maskPyramid.gaussianBlur.sigma = sigma;
    maskPyramid.gaussianBlur.size = size;
    sourcePyramid.gaussianPyramid.gaussianBlur.sigma = sigma;
    sourcePyramid.gaussianPyramid.gaussianBlur.size = size;
    destPyramid.gaussianPyramid.gaussianBlur.sigma = sigma;
    destPyramid.gaussianPyramid.gaussianBlur.size = size;

    strcpy(this->fileName, newFileName);
    if(strcmp(this->fileName, "")!=0) 
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
    	filenameChanged = false;        
    }    
}

void MultiResComposite::Reconstruct(GLuint textureOut, GLuint sourceTexture, GLuint destTexture, GLuint mask, int width, int height)
{
 	glUseProgram(shader);
	
    //Bind image A
    glUniform1i(glGetUniformLocation(shader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    //Bind sampler
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    glUniform1i(glGetUniformLocation(shader, "sourceTexture"), 0);
    
    //Bind sampler
	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, destTexture);
    glUniform1i(glGetUniformLocation(shader, "destTexture"), 1);
    
    //Bind sampler
	glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mask);
    glUniform1i(glGetUniformLocation(shader, "maskTexture"), 2);
    
     
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);       
}


void MultiResComposite::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    if(maskTexture.width != imageProcessStack->width || maskTexture.height != imageProcessStack->height || !maskTexture.loaded)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR; 
        if(maskTexture.loaded) maskTexture.Unload();
        maskTexture = GL_TextureFloat(imageProcessStack->width, imageProcessStack->height, tci);
        maskData.resize(maskTexture.width * maskTexture.height);
    }

    if(drawingMask)
    {
        glUseProgram(viewMaskShader);
        SetUniforms();

        glUniform1i(glGetUniformLocation(viewMaskShader, "textureIn"), 0); //program must be active
        glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "textureOut"), 1); //program must be active
        glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "mask"), 2); //program must be active
        glBindImageTexture(2, maskTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glUniform1i(glGetUniformLocation(viewMaskShader, "sourceTexture"), 3); //program must be active
        glBindImageTexture(3, texture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
        glUseProgram(0);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    else
    {
        sourcePyramid.Build(texture.glTex, texture.width, texture.height);
        destPyramid.Build(textureIn, width, height);
        maskPyramid.Build(maskTexture.glTex, maskTexture.width, maskTexture.height);
        
        sourcePyramid.ClearTexture(textureOut, width, height);
        
        for(int i=sourcePyramid.gaussianPyramid.depth-1; i>=0; i--)
        {
            Reconstruct(textureOut, 
                        sourcePyramid.gaussianPyramid.pyramid[i].glTex,
                        destPyramid.gaussianPyramid.pyramid[i].glTex,
                        maskPyramid.pyramid[i].glTex,
                        width, height);

        }
    }
}

void MultiResComposite::SetUniforms()
{

}

bool MultiResComposite::RenderGui()
{
    bool changed=false;
    selected=true;
    
    float sigma = maskPyramid.gaussianBlur.sigma;
    int size = maskPyramid.gaussianBlur.size;
    changed |= ImGui::SliderFloat("Sigma", &sigma, 0, 10);
    changed |= ImGui::SliderInt("Size", &size, 3, 21);

    maskPyramid.gaussianBlur.sigma = sigma;
    maskPyramid.gaussianBlur.size = size;
    sourcePyramid.gaussianPyramid.gaussianBlur.sigma = sigma;
    sourcePyramid.gaussianPyramid.gaussianBlur.size = size;
    destPyramid.gaussianPyramid.gaussianBlur.sigma = sigma;
    destPyramid.gaussianPyramid.gaussianBlur.size = size;

    ImGui::Text("Mask");
    changed |= ImGui::Checkbox("Draw Mask", &drawingMask);
    if(drawingMask)
    {
        changed |= ImGui::SliderInt("Radius", &radius, 1, 25);
        changed |= ImGui::Checkbox("Add", &adding);
    }

    ImGui::Separator();
    filenameChanged |= ImGui::InputText("File Name", fileName, IM_ARRAYSIZE(fileName));
    if(filenameChanged)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
        
		changed=true;
		filenameChanged = false;
    }

    return changed;
}


void MultiResComposite::Unload()
{
    maskTexture.Unload();
    texture.Unload();
    glDeleteProgram(viewMaskShader);
}

bool MultiResComposite::MouseMove(float x, float y) 
{
    if(x<0 || !selected && !drawingMask) return false;

    bool drawChanged=false;
    glm::vec2 currentMousPos(x, y);
    
    if(mousePressed && previousMousPos != currentMousPos)
    {
        glm::vec2 diff = currentMousPos - previousMousPos;
        float diffLength = std::ceil(glm::length(diff));
        for(float i=0; i<diffLength; i++)
        {
            float t = i / diffLength;
            glm::vec2 delta = t * diff;
            glm::ivec2 c = previousMousPos + delta;
            for(int ky=-radius; ky <=radius; ky++)
            {
                for(int kx=-radius; kx <=radius; kx++)
                {
                    if((ky*ky + kx+kx) < radius*radius)
                    {
                        glm::ivec2 coord(c.x + kx, c.y + ky);
                        if(coord.x <0 || coord.y < 0 || coord.x >= maskTexture.width || coord.y >= maskTexture.height) continue;
                        int inx = coord.y * maskTexture.width + coord.x;
                        maskData[inx] = adding ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1);
                    }
                }
            }
        }


        glBindTexture(GL_TEXTURE_2D, maskTexture.glTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, maskTexture.width, maskTexture.height, 0, GL_RGBA, GL_FLOAT, maskData.data());
        glBindTexture(GL_TEXTURE_2D, 0);        

        drawChanged=true;
    }

    previousMousPos = currentMousPos;

    selected=false;

    return drawChanged;
}

bool MultiResComposite::MousePressed() 
{
    mousePressed=true;
    return false;
}

bool MultiResComposite::MouseReleased() 
{
    mousePressed=false;
    return false;
}


//

//
//------------------------------------------------------------------------
AddColor::AddColor(bool enabled) : ImageProcess("AddColor", "shaders/AddColor.glsl", enabled)
{
}


void AddColor::SetUniforms()
{
    glUniform3fv(glGetUniformLocation(shader, "color"),1, glm::value_ptr(color));    
}

bool AddColor::RenderGui()
{
    bool changed=false;
    changed |= ImGui::ColorEdit3("color", &color[0]);
    return changed;
}
//

//
//------------------------------------------------------------------------
LaplacianOfGaussian::LaplacianOfGaussian(bool enabled) : ImageProcess("LaplacianOfGaussian", "shaders/LaplacianOfGaussian.glsl", enabled)
{
    kernel.resize(maxSize * maxSize);
    glGenBuffers(1, (GLuint*)&kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     
}

void LaplacianOfGaussian::RecalculateKernel()
{
    int halfSize = (int)std::floor(size / 2.0f);
    float sigma2 = sigma*sigma;
    float s = 2.0f * sigma2;
    float sum = 0.0f;
    
    for (int x = -halfSize; x <= halfSize; x++) {
        for (int y = -halfSize; y <= halfSize; y++) {
            int flatInx = (y + halfSize) * size + (x + halfSize);
            float x2 = (float)(x*x);
            float y2 = (float)(y*y);
            
            float a = -1 / (PI * sigma2 * sigma2);
            float b = 1 - (x2 + y2) /s;
            float c = exp(-(x2 + y2) / s);
            // float a = (x2 + y2 - s) / (sigma2 * sigma2);
			kernel[flatInx] = (a * b * c);

            sum += kernel[flatInx];
        }
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    shouldRecalculateKernel=false;
}

void LaplacianOfGaussian::SetUniforms()
{
    if(shouldRecalculateKernel) RecalculateKernel();

    glUniform1i(glGetUniformLocation(shader, "size"), size);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
}

bool LaplacianOfGaussian::RenderGui()
{
    bool changed=false;
    shouldRecalculateKernel |= ImGui::SliderInt("Size", &size, 1, maxSize);
    shouldRecalculateKernel |= ImGui::SliderFloat("Sigma", &sigma, 0, 2);

    changed |= shouldRecalculateKernel;
    return changed;
}

void LaplacianOfGaussian::Unload()
{
    glDeleteBuffers(1, &kernelBuffer);
}
//

//
//------------------------------------------------------------------------
DifferenceOfGaussians::DifferenceOfGaussians(bool enabled) : ImageProcess("DifferenceOfGaussians", "shaders/DifferenceOfGaussians.glsl", enabled)
{
    kernel.resize(maxSize * maxSize);
    glGenBuffers(1, (GLuint*)&kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     
}

void DifferenceOfGaussians::RecalculateKernel()
{
    int halfSize = (int)std::floor(size / 2.0f);

    std::vector<float> kernel1(size * size);
    {
        float s = 2.0f * sigma1;
        for (int x = -halfSize; x <= halfSize; x++) {
            for (int y = -halfSize; y <= halfSize; y++) {
                int flatInx = (y + halfSize) * size + (x + halfSize);
                float x2 = (float)(x*x);
                float y2 = (float)(y*y);
                double num = (exp(-(x2+y2) / s));
                double denom = (PI * s);
                kernel1[flatInx] =  (float)(num / denom);
            }
        }
    }

    std::vector<float> kernel2(size * size);
    {
       float s = 2.0f * sigma2;
       for (int x = -halfSize; x <= halfSize; x++) {
            for (int y = -halfSize; y <= halfSize; y++) {
                int flatInx = (y + halfSize) * size + (x + halfSize);
                float x2 = (float)(x*x);
                float y2 = (float)(y*y);
                double num = (exp(-(x2+y2) / s));
                double denom = (PI * s);
                kernel2[flatInx] =  (float)(num / denom);
            }
        }
    }

    for(int i=0; i<size * size; i++)
    {
        kernel[i] = kernel2[i]-kernel1[i];
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    shouldRecalculateKernel=false;
}

void DifferenceOfGaussians::SetUniforms()
{
    if(shouldRecalculateKernel) RecalculateKernel();

    glUniform1i(glGetUniformLocation(shader, "size"), size);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
}

bool DifferenceOfGaussians::RenderGui()
{
    bool changed=false;
    shouldRecalculateKernel |= ImGui::SliderInt("Size", &size, 1, maxSize);
    shouldRecalculateKernel |= ImGui::SliderFloat("Sigma", &sigma1, 0, 10);
    sigma2 = sigma1*2;
    changed |= shouldRecalculateKernel;
    return changed;
}

void DifferenceOfGaussians::Unload()
{
    glDeleteBuffers(1, &kernelBuffer);
}
//

//
//------------------------------------------------------------------------
CannyEdgeDetector::CannyEdgeDetector(bool enabled) : ImageProcess("CannyEdgeDetector", "shaders/GaussianBlur.glsl", enabled)
{
    kernel.resize(maxSize * maxSize);
    glGenBuffers(1, (GLuint*)&kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     


   CreateComputeShader("shaders/cannyGradient.glsl", &gradientShader);
   CreateComputeShader("shaders/cannyEdge.glsl", &edgeShader);
   CreateComputeShader("shaders/cannyThreshold.glsl", &thresholdShader);
   CreateComputeShader("shaders/cannyHysteresis.glsl", &hysteresisShader);
}

void CannyEdgeDetector::RecalculateKernel()
{
    int halfSize = (int)std::floor(size / 2.0f);
    double r, s = 2.0 * sigma * sigma;
    double sum = 0.0;
    
    for (int x = -halfSize; x <= halfSize; x++) {
        for (int y = -halfSize; y <= halfSize; y++) {
            int flatInx = (y + halfSize) * size + (x + halfSize);
            r = (float)sqrt(x * x + y * y);
			double num = (exp(-(r * r) / s));
			double denom = (PI * s);
            kernel[flatInx] =  (float)(num / denom);
            sum += kernel[flatInx];
        }
    }

    // normalising the Kernel
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            int flatInx = (i) * size + (j);
            kernel[flatInx] /= (float)sum;
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    shouldRecalculateKernel=false;
}

void CannyEdgeDetector::SetUniforms()
{
    if(shouldRecalculateKernel) RecalculateKernel();

    glUniform1i(glGetUniformLocation(shader, "size"), size);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
}

bool CannyEdgeDetector::RenderGui()
{
    bool changed=false;
    shouldRecalculateKernel |= ImGui::SliderInt("Size", &size, 1, maxSize);
    shouldRecalculateKernel |= ImGui::SliderFloat("Sigma", &sigma, 1, 10);
    
    shouldRecalculateKernel |= ImGui::SliderFloat("Low Threshold", &threshold, 0, 0.33f);

    changed |= ImGui::DragInt("Output", &outputStep, 1, 0, 4);

    changed |= shouldRecalculateKernel;
    return changed;
}

void CannyEdgeDetector::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    if(blurTexture.width != width || blurTexture.height != height || !blurTexture.loaded)
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.srgb=true;
        tci.minFilter = GL_LINEAR;
        tci.magFilter = GL_LINEAR;        
        blurTexture = GL_TextureFloat(width, height, tci);        
        gradientTexture = GL_TextureFloat(width, height, tci);        
        edgeTexture = GL_TextureFloat(width, height, tci);        
        thresholdTexture = GL_TextureFloat(width, height, tci);        
    }

	//Blur

	glUseProgram(shader);
	SetUniforms();
    glUniform1i(glGetUniformLocation(shader, "textureIn"), 0); //program must be active
    glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	
    glUniform1i(glGetUniformLocation(shader, "textureOut"), 1); //program must be active
    glBindImageTexture(1, (outputStep==0) ? textureOut : blurTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glUseProgram(0);

    if(outputStep==0) return;
	
    //Gradient
    glUseProgram(gradientShader);
    glUniform1i(glGetUniformLocation(gradientShader, "textureIn"), 0); //program must be active
    glBindImageTexture(0, blurTexture.glTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	
    glUniform1i(glGetUniformLocation(gradientShader, "textureOut"), 1); //program must be active
    glBindImageTexture(1, (outputStep==1) ? textureOut : gradientTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glUseProgram(0);
	
    if(outputStep==1) return;

    //Edge
    glUseProgram(edgeShader);
    glUniform1i(glGetUniformLocation(edgeShader, "textureIn"), 0); //program must be active
    glBindImageTexture(0, gradientTexture.glTex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	
    glUniform1i(glGetUniformLocation(edgeShader, "textureOut"), 1); //program must be active
    glBindImageTexture(1, (outputStep==2) ? textureOut : edgeTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glUseProgram(0);

    if(outputStep==2) return;

    //Threshold
    glUseProgram(thresholdShader);
    glUniform1i(glGetUniformLocation(thresholdShader, "textureIn"), 0); //program must be active
    glBindImageTexture(0, edgeTexture.glTex , 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	
    glUniform1i(glGetUniformLocation(thresholdShader, "textureOut"), 1); //program must be active
    glBindImageTexture(1, (outputStep==3) ? textureOut : thresholdTexture.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glUniform1f(glGetUniformLocation(thresholdShader, "threshold"), threshold); //program must be active
    
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glUseProgram(0);

    if(outputStep==3) return;

    //Histeresis
    glUseProgram(hysteresisShader);
    glUniform1i(glGetUniformLocation(hysteresisShader, "textureIn"), 0); //program must be active
    glBindImageTexture(0, thresholdTexture.glTex , 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	
    glUniform1i(glGetUniformLocation(hysteresisShader, "textureOut"), 1); //program must be active
    glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glUseProgram(0);

}
void CannyEdgeDetector::Unload()
{
    glDeleteProgram(gradientShader);
    glDeleteProgram(edgeShader);
    glDeleteProgram(thresholdShader);
    glDeleteProgram(hysteresisShader);
    blurTexture.Unload();
    gradientTexture.Unload();
    edgeTexture.Unload();
    thresholdTexture.Unload();
    
}

//

//
//------------------------------------------------------------------------
ArbitraryFilter::ArbitraryFilter(bool enabled) : ImageProcess("ArbitraryFilter", "shaders/ArbitraryFilter.glsl", enabled)
{
    kernel.resize(maxSize * maxSize, 1);
    normalizedKernel.resize(maxSize * maxSize, 1);

    glGenBuffers(1, (GLuint*)&kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, kernel.size() * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);     
}

void ArbitraryFilter::RecalculateKernel()
{
    // kernel.resize(sizeX * sizeY, 1);
    
    // // normalising the Kernel
    if(normalize)
    {
        float sum=0;
        for(int i=0; i<sizeX * sizeY; i++)
        {
            sum += std::abs(kernel[i]);
        }
        for(int i=0; i< sizeX * sizeY; i++)
        {
            normalizedKernel[i] = kernel[i] / sum;
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(float), normalizedKernel.data(), GL_DYNAMIC_COPY); 
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
    else
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(float), kernel.data(), GL_DYNAMIC_COPY); 
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    
    shouldRecalculateKernel=false;
}

void ArbitraryFilter::SetUniforms()
{
    if(shouldRecalculateKernel) RecalculateKernel();

    glUniform1i(glGetUniformLocation(shader, "sizeX"), sizeX);
    glUniform1i(glGetUniformLocation(shader, "sizeY"), sizeY);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, kernelBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, kernelBuffer);
}

bool ArbitraryFilter::RenderGui()
{
    bool changed=false;
    
    shouldRecalculateKernel |= ImGui::Checkbox("Normalize", &normalize);

    shouldRecalculateKernel |= ImGui::SliderInt("SizeX", &sizeX, 1, maxSize);
    shouldRecalculateKernel |= ImGui::SliderInt("SizeY", &sizeY, 1, maxSize);

    int inx=0;
    for(int y=0; y<sizeY; y++)
    {
        for(int x=0; x<sizeX; x++)
        {
            ImGui::PushID(inx);
            ImGui::SetNextItemWidth(40); shouldRecalculateKernel |= ImGui::DragFloat("", &kernel[inx], 0.05f); 
            if(x < sizeX-1) ImGui::SameLine();
            inx++;
            ImGui::PopID();
        }
    }
    
    changed |= shouldRecalculateKernel;
    return changed;
}
//

//
//------------------------------------------------------------------------
GammaCorrection::GammaCorrection(bool enabled) : ImageProcess("GammaCorrection", "shaders/GammaCorrection.glsl", enabled)
{}

void GammaCorrection::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "gamma"), gamma);
}

bool GammaCorrection::RenderGui()
{
    bool changed=false;
    changed |= ImGui::SliderFloat("Gamma", &gamma, 0.01f, 5);
    return changed;
}
//

//
//------------------------------------------------------------------------
ColorDistance::ColorDistance(bool enabled) : ImageProcess("ColorDistance", "shaders/ColorDistance.glsl", enabled)
{
    
}

void ColorDistance::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "clipDistance"), distance);
    glUniform3fv(glGetUniformLocation(shader, "clipColor"), 1, glm::value_ptr(color));
}

bool ColorDistance::RenderGui()
{
    bool changed=false;
    changed |= ImGui::ColorEdit3("Color", &color[0]);
    changed |= ImGui::DragFloat("Distance", &distance, 0.001f);
    return changed;
}
//

//
//------------------------------------------------------------------------
EdgeLinking::EdgeLinking(bool enabled) : ImageProcess("EdgeLinking", "", enabled)
{
    cannyEdgeDetector = new CannyEdgeDetector(true);
}

void EdgeLinking::Unload()
{
    glDeleteProgram(shader);
    cannyEdgeDetector->Unload();
    delete cannyEdgeDetector;
}

void EdgeLinking::SetUniforms()
{
}

bool EdgeLinking::RenderGui()
{
    bool changed=false;
    
    ImGui::Text("Canny Edge Detector");
    cannyChanged = cannyEdgeDetector->RenderGui();

    changed |= cannyChanged;

    ImGui::Separator();
    ImGui::Text("Edge Linking");

    changed |= ImGui::Checkbox("Apply Process", &doProcess);
    changed |= ImGui::SliderInt("Window Size", &windowSize, 1, 32);
    changed |= ImGui::DragFloat("Magnitude Threshold", &magnitudeThreshold, 0.001f, 0, 100);
    changed |= ImGui::DragFloat("Angle Threshold", &angleThreshold, 0.001f, 0, 100);
    
    return changed;
}


void EdgeLinking::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    //Only recompute canny when its params changed. //Output it into textureOut, act as a tmp texture that we'll write again after
    cannyEdgeDetector->Process(textureIn, textureOut, width, height);

    //Read back gradient
    gradientData.resize(width * height, glm::vec4(0));
    glBindTexture(GL_TEXTURE_2D, cannyEdgeDetector->gradientTexture.glTex);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    gradientData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    //Read back edges
    edgeData.resize(width * height, glm::vec4(0));
    glBindTexture(GL_TEXTURE_2D, textureOut);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    edgeData.data());
    glBindTexture(GL_TEXTURE_2D, 0);


    int halfWindowSize = windowSize/2;

    linkedEdgeData.resize(width * height);
    std::fill(linkedEdgeData.begin(), linkedEdgeData.end(), glm::vec4(0));

    glm::ivec2 pixelCoord;
    for(pixelCoord.y=0; pixelCoord.y<height; pixelCoord.y++)
    {
        for(pixelCoord.x=0; pixelCoord.x<width; pixelCoord.x++)
        {

            int inx = pixelCoord.y * width + pixelCoord.x;
            float pixelEdge = edgeData[inx].x;
            
            //Copy the edgeData to the output in all cases
            linkedEdgeData[inx] = edgeData[inx];

            if(!doProcess) continue;
            
            if(pixelEdge>0)
            {
                float pixelMagnitude = gradientData[inx].x;
                float pixelAngle = gradientData[inx].y;

                glm::ivec2 windowCoord;
                for(windowCoord.y=-halfWindowSize; windowCoord.y < halfWindowSize; windowCoord.y++)
                {
                    for(windowCoord.x=-halfWindowSize; windowCoord.x < halfWindowSize; windowCoord.x++)
                    {
                        glm::ivec2 coord = pixelCoord + windowCoord;
                        if(coord.x < 0 || coord.y < 0 || coord.x >=width || coord.y >=height)continue;

                        int coordInx = coord.y * width + coord.x;
                        float windowEdge = edgeData[coordInx].x;
                        if(windowEdge>0) //If we find another edge in the window
                        {

                            float windowMagnitude = gradientData[coordInx].x;
                            float windowAngle = gradientData[coordInx].y;
                            
                            if(std::abs(windowMagnitude - pixelMagnitude) < magnitudeThreshold && std::abs(windowAngle - pixelAngle) < angleThreshold)
                            {
                                //Add an edge between pixelCoord and windowCoord
                                DrawLine(pixelCoord, coord, linkedEdgeData, width, height, glm::vec4(1));
                            }
                        }
                    }
                }
            }

        }    
    }




    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, linkedEdgeData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
//
//

//
//------------------------------------------------------------------------
RegionGrow::RegionGrow(bool enabled) : ImageProcess("RegionGrow", "", enabled)
{
}

void RegionGrow::Unload()
{
    glDeleteProgram(shader);
}

void RegionGrow::SetUniforms()
{
}

bool RegionGrow::RenderGui()
{
    bool changed=false;
    
    ImGuiIO& io = ImGui::GetIO();
    
    if(io.MouseClicked[0] && !ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
    {
        changed=true;
        clickedPoint = glm::vec2(io.MousePos.x, io.MousePos.y) / glm::vec2(io.DisplaySize.x, io.DisplaySize.y);
    }
    else
    {
        changed=false;
        clickedPoint = glm::vec2(-1, -1);
    }

    ImGui::SliderFloat("Threshold", &threshold, 0, 1);
    ImGui::Combo("Output Type", (int*)&outputType, "Add\0Mask\0Isolate\0\0");
    
    return changed;
}


void RegionGrow::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    inputData.resize(width * height);
    mask.resize(width * height);
    tracker.resize(width * height);
    std::fill(mask.begin(), mask.end(), glm::vec4(0,0,0,1));
    std::fill(tracker.begin(), tracker.end(), false);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);



    if(clickedPoint.x >=0) 
    {
        glm::ivec2 clickedPointTextureSpace = glm::ivec2(clickedPoint * glm::vec2(width, height));
        std::stack<glm::ivec2> pointsToExplore;

        glm::vec3 currentPointColor = inputData[clickedPointTextureSpace.y * width + clickedPointTextureSpace.x];
        
        pointsToExplore.push(clickedPointTextureSpace);
        while(pointsToExplore.size() !=0)
        {
            glm::ivec2 currentPoint = pointsToExplore.top();
            pointsToExplore.pop();


            for(int y=-1; y<=1; y++)
            {
                for(int x=-1; x<=1; x++)
                {
                    glm::ivec2 coord = currentPoint + glm::ivec2(x, y);
					if (coord.x >= 0 && coord.x < width && coord.y >= 0 && coord.y < height)
					{
						glm::vec3 color = inputData[coord.y * width + coord.x];
						if(glm::distance(color, currentPointColor) < threshold && tracker[coord.y * width + coord.x] == false)
						{
							pointsToExplore.push(coord);
                            tracker[coord.y * width + coord.x]=true;
                            if(outputType == OutputType::AddColorToImage)
                            {
							    inputData[coord.y * width + coord.x] = glm::vec4(1,0,0,1);
                            }
                            else if(outputType == OutputType::Mask)
                            {
                                mask[coord.y * width + coord.x] = glm::vec4(1,1,1,1);
                            }
                            else if(outputType == OutputType::Isolate)
                            {
                                mask[coord.y * width + coord.x] = inputData[coord.y * width + coord.x];
                            }
						}
					}
                }
            }
        }
    }

    if(outputType==OutputType::AddColorToImage)
    {
        glBindTexture(GL_TEXTURE_2D, textureOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, inputData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, textureOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, mask.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
//
//

//
//------------------------------------------------------------------------
KMeansCluster::KMeansCluster(bool enabled) : ImageProcess("KMeansCluster", "", enabled)
{
}

void KMeansCluster::Unload()
{
    glDeleteProgram(shader);
}

void KMeansCluster::SetUniforms()
{
}

bool KMeansCluster::RenderGui()
{
    bool changed=false;
    
    if(ImGui::Button("Process")) shouldProcess=true;
    changed |= ImGui::DragInt("Num Clusters", &numClusters);
    changed |= ImGui::Combo("OutputMode", (int*)&outputMode, "Random Color\0Cluster Color\0GrayScale\0\0");

    changed |= shouldProcess;
    return changed;
}


void KMeansCluster::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    inputData.resize(width * height);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if(shouldProcess)
    {
        //Initialize the clusters with random positions
        clusterPositions.resize(numClusters);
        for(int i=0; i<numClusters; i++)
        {
            clusterPositions[i] = glm::vec3(
                (float)rand() / (float)RAND_MAX,
                (float)rand() / (float)RAND_MAX,
                (float)rand() / (float)RAND_MAX
            );
        }

        clusterMapping.resize(numClusters);
        float totalDiff=100000;
        while(totalDiff > 0.1f)
        {
            //Clear the mapping
            for(int i=0; i<clusterMapping.size(); i++)
            {
                clusterMapping[i].clear();
            }
            
            //Associate each pixel with one cluster
            for(int i=0; i<inputData.size(); i++)
            {
                float closestDistance = 1e30f;
                int closestCluster=0;
                for(int j=0; j<numClusters; j++)
                {
                    glm::vec3 color = glm::vec3(inputData[i]);
                    float distance = glm::distance2(color, clusterPositions[j]);
                    
                    if(distance < closestDistance)
                    {
                        closestDistance=distance;
                        closestCluster=j;
                    }
                }
                clusterMapping[closestCluster].push_back(i);
            }

            //calculate new cluster positions;
            totalDiff = 0;
            for(int i=0; i<numClusters; i++)
            {
                glm::vec3 average(0);
                float inverseSize = 1.0f / (float)clusterMapping[i].size();
                for(int j=0; j<clusterMapping[i].size(); j++)
                {
                    average += glm::vec3(inputData[clusterMapping[i][j]]) * inverseSize;
                }

                float  diff = glm::distance2(clusterPositions[i], average);
                totalDiff += diff;
                clusterPositions[i] = average;
            }
        }

        for(int i=0; i<clusterMapping.size(); i++)
        {
            glm::vec3 clusterColor(0);
            if(outputMode==OutputMode::ClusterColor)
                clusterColor = clusterPositions[i];
            else if(outputMode==OutputMode::GrayScale)
            {
                float value = (float)i / (float)numClusters;
                clusterColor = glm::vec3(value);
            }
            else if(outputMode==OutputMode::RandomColor)
            {
                clusterColor = glm::vec3(
                    (float)rand() / (float)RAND_MAX,
                    (float)rand() / (float)RAND_MAX,
                    (float)rand() / (float)RAND_MAX
                );
            }
            for(int j=0; j<clusterMapping[i].size(); j++)
            {
                // inputData[clusterMapping[i][j]] = glm::vec4(value, value, value, 1);
                inputData[clusterMapping[i][j]] = glm::vec4(clusterColor, 1);
            }
        }

        shouldProcess=false;
    }


    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
//
//

//
//------------------------------------------------------------------------
ErrorDiffusionHalftoning::ErrorDiffusionHalftoning(bool enabled) : ImageProcess("ErrorDiffusionHalftoning", "", enabled)
{
    RecalculateMask();
}

void ErrorDiffusionHalftoning::Unload()
{
    glDeleteProgram(shader);
}

void ErrorDiffusionHalftoning::SetUniforms()
{
}

bool ErrorDiffusionHalftoning::RenderGui()
{
    bool changed=false;
    
    if(ImGui::Button("Process")) shouldProcess=true;
    shouldProcess |= ImGui::SliderFloat("Threshold", &threshold, 0, 1);
    shouldProcess |= ImGui::Checkbox("GrayScale", &grayScale);
    shouldRecalculateMask |= ImGui::Combo("Mask Type", &masktype, "Floyd Steinberg\0 Stucky\0\0");
    
    shouldProcess |= shouldRecalculateMask;
    changed |= shouldProcess;
    return changed;
}


void ErrorDiffusionHalftoning::RecalculateMask()
{
    if(masktype==0)
    {
        float t = 1.f/16.f;
        mask = 
        {
            t * 1, t * 3, t * 5, t * 1
        };

        maskIndices = 
        {
            glm::ivec2( 1, 0),
            glm::ivec2(-1, 1),
            glm::ivec2( 0, 1),
            glm::ivec2( 1, 1),
        };
    }
    else if(masktype==1)
    {
        float t = 1.f/42.f;
        mask = 
        {
                                 t * 8, t * 4, 
            t * 2, t * 4, t * 8, t * 4, t * 2,
            t * 1, t * 2, t * 4, t * 2, t * 1,
        };

        maskIndices = 
        {
                                                                     glm::ivec2( 1, 0), glm::ivec2( 2, 0),
            glm::ivec2(-2, 1), glm::ivec2(-1, 1), glm::ivec2( 0, 1), glm::ivec2( 1, 1), glm::ivec2( 2, 1), 
            glm::ivec2(-2, 2), glm::ivec2(-1, 2), glm::ivec2( 0, 2), glm::ivec2( 1, 2), glm::ivec2( 2, 2),
        };        
    }
}

void ErrorDiffusionHalftoning::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    if(shouldRecalculateMask)
    {
        RecalculateMask();
        shouldRecalculateMask=false;
    }
    inputData.resize(width * height);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if(shouldProcess)
    {
        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                //Serpentine loop through the image
                bool pairLine = (y%2==0);
                int sampleX = pairLine ? x : width - 1 - x;
                // int sampleX = x;
                
                //Get Color
                int inx = y * width + sampleX;
                glm::vec3 color = inputData[inx];
                if(grayScale) color = GrayScale2Color(Color2GrayScale(color), 1);

                //quantize
                glm::vec3 newColor(0);
                if(color.r<threshold) newColor.r=0;
                else                    newColor.r=1;
                
                if(color.g<threshold) newColor.g=0;
                else                    newColor.g=1;
                
                if(color.b<threshold) newColor.b=0;
                else                    newColor.b=1;
                
                inputData[inx] = glm::vec4(newColor,1);

                //Get error
                glm::vec3 error = color -  newColor;

                //do the halftoning
                for(int i=0; i<maskIndices.size(); i++)
                {
                    glm::ivec2 coord = glm::ivec2(sampleX, y) + maskIndices[i];
                    int maskInx = coord.y * width + coord.x;
                    if(maskInx <0 || maskInx>inputData.size()) continue;
                    
                    inputData[maskInx] += glm::vec4(error * mask[i], 0);
                }
            }
        }
       
        shouldProcess=false;
    }


    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
//
//

//
//------------------------------------------------------------------------
RegionProperties::RegionProperties(bool enabled) : ImageProcess("RegionProperties", "", enabled)
{
}

void RegionProperties::Unload()
{
    glDeleteProgram(shader);
}

void RegionProperties::SetUniforms()
{
}

bool RegionProperties::RenderGui()
{
    bool changed=false;
    
    if(ImGui::Button("Process")) shouldProcess=true;
    changed |= shouldProcess;

    ImGui::Checkbox("Compute Skeleton", &calculateSkeleton);

    if(ImGui::CollapsingHeader("Regions", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for(int i=0; i<regions.size(); i++)
        {
            ImGui::PushID(i);
            if(ImGui::TreeNodeEx("Region", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Display", &regions[i].renderGui);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
    return changed;
}

bool RegionProperties::RenderOutputGui()
{
    bool changed=false;
    // float width = ImGui::GetWindowContentRegionWidth();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    glm::vec2 margin = imageProcessStack->outputGuiStart;
    for(int i=0; i<regions.size(); i++)
    {
        if(regions[i].renderGui)
        {
            char regionName[80];
            sprintf(regionName, "Region %d", i);
            ImGui::Begin(regionName, nullptr, ImGuiWindowFlags_NoMove);
            
            ImVec2 pos = ImVec2((float)regions[i].center.x + margin.x, (float)regions[i].center.y + margin.y);
            glm::vec2 size = regions[i].boundingBox.maxBB - regions[i].boundingBox.minBB;
            
            ImGui::SetWindowPos(ImVec2(pos.x, pos.y - size.y/2 -50));
            
            ImGui::PushID(i);
            if(ImGui::CollapsingHeader("Info"))
            {
                ImGui::Text("Area : %f", regions[i].area);
                ImGui::Text("Perimeter : %f", regions[i].perimeter);
                ImGui::Text("Compactness : %f", regions[i].compactness);
                ImGui::Text("circularity : %f", regions[i].circularity);
                ImGui::Text("size : %fx%f", size.x, size.y);            
            }
            ImGui::PopID();

            ImGui::Checkbox("Draw BB", &regions[i].drawBB);
            if(regions[i].drawBB)
            {
                drawList->AddRect(
                    ImVec2(regions[i].boundingBox.minBB.x + margin.x, regions[i].boundingBox.minBB.y + margin.y), 
                    ImVec2(regions[i].boundingBox.maxBB.x + margin.x, regions[i].boundingBox.maxBB.y + margin.y), IM_COL32(0,0,255,255)
                );
            }
            
            ImGui::Checkbox("Draw Axes", &regions[i].DrawAxes);
            if(regions[i].DrawAxes)
            {
                glm::vec2 axis1 = glm::normalize(regions[i].eigenVector1) * 2 * sqrt(regions[i].eigenValue1) ;
                drawList->AddLine(
                    ImVec2(pos),
                    ImVec2(pos.x + axis1.x, pos.y + axis1.y),
                    IM_COL32(255,0,0,255)
                );
                glm::vec2 axis2 = glm::normalize(regions[i].eigenVector2) * 2 * sqrt(regions[i].eigenValue2) ;
                drawList->AddLine(
                    ImVec2(pos),
                    ImVec2(pos.x + axis2.x, pos.y + axis2.y),
                    IM_COL32(0,255,0,255)
                );
            }
            
            ImGui::End();
        }
    }

    return changed;
}

int RegionProperties::GetStartIndex(glm::ivec2 b, glm::ivec2 c)
{
    glm::ivec2 diff = c - b;
    if(diff == glm::ivec2(-1, 0)) return 0; //0:Left 
    if(diff == glm::ivec2(-1,-1)) return 1; //1:top left
    if(diff == glm::ivec2( 0,-1)) return 2; //2:top
    if(diff == glm::ivec2( 1,-1)) return 3; //3:top right
    if(diff == glm::ivec2( 1, 0)) return 4; //4:right
    if(diff == glm::ivec2( 1, 1)) return 5; //5:bottom right
    if(diff == glm::ivec2( 0, 1)) return 6; //6:bottom
    if(diff == glm::ivec2(-1, 1)) return 7; //7:bottom left
    return 0;
}



void RegionProperties::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    inputData.resize(width * height);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //Find all the different regions
    std::vector<bool> pixelProcessed(width*height, false);
    if(shouldProcess)
    {
		regions.clear();
        glm::ivec2 currentPixel;
        for(currentPixel.y=0; currentPixel.y<height; currentPixel.y++)
        {
            for(currentPixel.x=0; currentPixel.x<width; currentPixel.x++)
            {
                int inx = currentPixel.y * width + currentPixel.x;
                if(inputData[inx].r >0 && !pixelProcessed[inx]) //Found a white pixel, and we haven't processed it yet
                {
                    Region region = {};

                    //Find all the points inside the current region
                    std::stack<glm::ivec2> pointsToExplore;
                    pointsToExplore.push(currentPixel);
                    while(pointsToExplore.size() !=0)
                    {
                        glm::ivec2 currentPoint = pointsToExplore.top();
                        pointsToExplore.pop();
                        for(int y=-1; y<=1; y++)
                        {
                            for(int x=-1; x<=1; x++)
                            {
                                glm::ivec2 coord = currentPoint + glm::ivec2(x, y);
                                if (coord.x >= 0 && coord.x < width && coord.y >= 0 && coord.y < height)
                                {
                                    if(inputData[coord.y * width + coord.x].r >0 && !pixelProcessed[coord.y * width + coord.x])
                                    {
                                        pointsToExplore.push(coord);
                                        pixelProcessed[coord.y * width + coord.x]=true;
                                        region.points.push_back(coord);
                                    }
                                }
                            }
                        }
                    }
                    regions.push_back(region);
                }
            }
        }

        
        for(int j=0; j<regions.size(); j++)
        {   
            //Calculate the region bounding box
            regions[j].boundingBox.minBB = glm::ivec2(width, height);
            regions[j].boundingBox.maxBB = glm::ivec2(0);
            for(int i=0; i<regions[j].points.size(); i++)
            {
                regions[j].boundingBox.minBB = glm::min(regions[j].points[i],regions[j].boundingBox.minBB);
                regions[j].boundingBox.maxBB = glm::max(regions[j].points[i],regions[j].boundingBox.maxBB);
            }

            std::vector<point> outlinePoints;
            //Find the first point
            bool shouldBreak = false;
            for(int y=regions[j].boundingBox.minBB.y; y<regions[j].boundingBox.maxBB.y; y++)
            {
                for(int x=regions[j].boundingBox.minBB.x; x<regions[j].boundingBox.maxBB.x; x++)
                {
                    if(inputData[y * width + x].x != 0)
                    {
                        outlinePoints.push_back(
                            {
                                glm::ivec2(x, y),
                                glm::ivec2(x-1, y)
                            }
                        );
                        shouldBreak = true;
                        break;
                    }
                }
                if (shouldBreak)break;
            }
            
            //Find the ordered list of points
            point *currentPoint = &outlinePoints[outlinePoints.size()-1];
            point firstPoint = *currentPoint;
            do {
                glm::ivec2 prevCoord;
                int startInx = GetStartIndex(currentPoint->b, currentPoint->c);
                
                for(int i=startInx; i<startInx + directions.size(); i++)
                {
                    int dirInx = i % directions.size();

                    glm::ivec2 checkCoord = currentPoint->b + directions[dirInx];
                    float checkValue = inputData[checkCoord.y * width + checkCoord.x].x;
                    if(checkValue>0)
                    {
                        point newPoint = 
                        {
                            checkCoord,
                            prevCoord,
                        };
                        outlinePoints.push_back(newPoint);
                        break;
                    }
                    prevCoord = checkCoord;
                }

                currentPoint = &outlinePoints[outlinePoints.size()-1];
            }
            while(currentPoint->b != firstPoint.b && currentPoint->c != firstPoint.c);            

            ///Calculate all the properties
            regions[j].perimeter = (float)outlinePoints.size();
            regions[j].area = (float)regions[j].points.size();
            regions[j].compactness = (regions[j].perimeter * regions[j].perimeter) / regions[j].area;
            regions[j].circularity = (4 * PI * regions[j].area) / (regions[j].perimeter * regions[j].perimeter);
            
            for(int i=0; i<regions[j].points.size(); i++)
            {
                //Center
                regions[j].center += glm::uvec2(regions[j].points[i]);
            }
            regions[j].center /= regions[j].points.size();

            
            glm::mat2 covarianceMatrix(0);
            for(int i=0; i<regions[j].points.size(); i++)
            {
                glm::vec2 v = regions[j].points[i] - glm::ivec2(regions[j].center);
                covarianceMatrix[0][0] += v.x * v.x;
                covarianceMatrix[1][0] += v.y * v.x;
                covarianceMatrix[0][1] += v.y * v.x;
                covarianceMatrix[1][1] += v.y * v.y;
            }
            covarianceMatrix /= regions[j].points.size();
            
            float trace = covarianceMatrix[0][0] + covarianceMatrix[1][1];
            float halfTrace = trace/2.0f;
            float trace2 = trace*trace;

            float determinant = (covarianceMatrix[0][0] * covarianceMatrix[1][1]) - (covarianceMatrix[1][0] * covarianceMatrix[0][1]);
            
            float a = pow(trace2 / 4 - determinant, 0.5f);

            regions[j].eigenValue1 =  halfTrace + a;
            regions[j].eigenValue2 =  halfTrace - a;

            
            if(covarianceMatrix[0][1]!=0)
            {
                regions[j].eigenVector1 = glm::vec2(
                    regions[j].eigenValue1 - covarianceMatrix[1][1],
                    covarianceMatrix[0][1]
                );
                regions[j].eigenVector2 = glm::vec2(
                    regions[j].eigenValue2 - covarianceMatrix[1][1],
                    covarianceMatrix[0][1]
                );
            }
            else if(covarianceMatrix[0][1]!=0)
            {
                regions[j].eigenVector1 = glm::vec2(
                    covarianceMatrix[1][0],
                    regions[j].eigenValue1 - covarianceMatrix[0][0]
                );
                regions[j].eigenVector2 = glm::vec2(
                    covarianceMatrix[1][0],
                    regions[j].eigenValue2 - covarianceMatrix[0][0]
                );
            }
            else
            {
                regions[j].eigenVector1 = glm::vec2(1,0);
                regions[j].eigenVector1 = glm::vec2(0,1);
            }

            //Calculate skeleton
            if(calculateSkeleton)
            {
                //
                for(int i=0; i<regions[j].points.size(); i++)
                {
                    glm::ivec2 current = regions[j].points[i];

                    //Find shortest distance on the outline from the current point  
                    float shortestDistance1 = 10000000;
                    int shortestIndex1 = 0;
                    for(int k=0; k<outlinePoints.size(); k++)
                    {
                        glm::vec2 diff = current - outlinePoints[k].b;
                        float distance = glm::length2(diff);

                        //If distance is less than distance 1 : set distance1 to distance, and distance2 ot distance1
                        if(distance <= shortestDistance1)
                        {
                            shortestDistance1 = distance;
                            shortestIndex1 = k;
                        }
                    }
                    glm::vec2 pointToBorder1 = outlinePoints[shortestIndex1].b - current;

                    //find another shortest distance that is on the opposite side of the first one.
                    float shortestDistance2 = 10000000;
                    int shortestIndex2 = 0;
                    for(int k=0; k<outlinePoints.size(); k++)
                    {
                        glm::vec2 diff = current - outlinePoints[k].b;
                        float distance = glm::length2(diff);
                        //If distance is less than distance 1 : set distance1 to distance, and distance2 ot distance1
                        if(distance <= shortestDistance2)
                        {
                            glm::vec2 pointToBorder2 = outlinePoints[k].b - current;
                            float dot =glm::dot(pointToBorder1, pointToBorder2);
                            if(dot <0)
                            {
                                shortestDistance2 = distance;
                                shortestIndex2 = k;
                            }
                        }
                    }
                    
                    //If the 2 distances are close, we're on the skeleton
                    if(sqrt(shortestDistance2) - sqrt(shortestDistance1) <=1)
                    {
                        inputData[current.y * width + current.x] = glm::vec4(1,0,0,1);
                    }
                }
                
            }
        }

        shouldProcess=false;
    }


    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
//
//

//
//------------------------------------------------------------------------
SuperPixelsCluster::SuperPixelsCluster(bool enabled) : ImageProcess("SuperPixelsCluster", "", enabled)
{
}

void SuperPixelsCluster::Unload()
{
    glDeleteProgram(shader);
}

void SuperPixelsCluster::SetUniforms()
{
}

bool SuperPixelsCluster::RenderGui()
{
    bool changed=false;
    
    if(ImGui::Button("Process")) shouldProcess=true;
    changed |= ImGui::DragInt("Num Clusters", &numClusters);
    changed |= ImGui::DragFloat("C", &C, 0.001f, 0.1f, 1000);
    
    changed |= ImGui::Combo("OutputMode", (int*)&outputMode, "Random Color\0Cluster Color\0GrayScale\0\0");
    
    changed |= shouldProcess;
    return changed;
}


void SuperPixelsCluster::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    inputData.resize(width * height);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    clusterPositions.clear();
    if(shouldProcess)
    {
        float totalColorDiff=1000;
        float totalPositionDiff=1000;

        //Assign initial positions
        int numPerRow = (int)sqrt(numClusters);
        glm::ivec2 clusterSize(width / numPerRow, height / numPerRow);
        glm::ivec2 clusterHalfSize = clusterSize / 2;
        int clusterDiagSize = (int)sqrt(clusterSize.x * clusterSize.x + clusterSize.y * clusterSize.y);
        for(int y=0; y<numPerRow; y++)
        {
            for(int x=0; x<numPerRow; x++)
            {
                glm::vec2 jitter(
                    (float)rand() / (float)RAND_MAX - 0.5f,
                    (float)rand() / (float)RAND_MAX - 0.5f
                );
                jitter *= clusterHalfSize;
                
                glm::vec2 clusterPosition = glm::vec2(
                    x * clusterSize.x + clusterHalfSize.x,
                    y * clusterSize.y + clusterHalfSize.y
                    ) + jitter;
                glm::vec3 clusterColor = glm::vec3(inputData[(int)clusterPosition.y * width + (int)clusterPosition.x]);
                clusterPositions.push_back(
                    {
                        clusterColor,
                        clusterPosition
                    }
                );
            }
        }

        while(totalColorDiff > 0.1f)
        {


            //Clear mapping
            clusterMapping.resize(clusterPositions.size());
            for(int i=0; i<clusterMapping.size(); i++)
            {
                clusterMapping[i].clear();
            }

            //Assign clusters
            for(int y=0; y<height; y++)
            {
                for(int x=0; x<width; x++)
                {
                    glm::vec2 normalizedCoord(
                        (float)x / (float)width,
                        (float)y / (float)height
                    );

                    float minDistance = 1e30f;
                    int clusterInx=0;

                    int inx=  y * width +x;
                    glm::vec3 pixelColor = inputData[inx];
                    glm::vec2 pixelPosition(x, y);

                    glm::ivec2 pixelClusterCoord = glm::ivec2(normalizedCoord.x * numPerRow, normalizedCoord.y * numPerRow);
                    for(int cy=-1; cy<=1; cy++)
                    {
                        for(int cx=-1; cx<=1; cx++)
                        {
                            glm::ivec2 clusterCoord = pixelClusterCoord + glm::ivec2(cx, cy);
                            int i = clusterCoord.y * numPerRow + clusterCoord.x;
                            if(i <0 || i >= clusterPositions.size())continue;

                            glm::vec2 clusterPosition = clusterPositions[i].position;
                            float positionDistance = glm::distance(clusterPosition, pixelPosition);
                            if(positionDistance < clusterDiagSize)
                            {
                                positionDistance /= (float)clusterDiagSize;

                                glm::vec3 clusterColor = clusterPositions[i].color;
                                float colorDistance = glm::distance(clusterColor, pixelColor) / C;
                                
                                //Calculate spatial distance
                                float distance = sqrt(positionDistance * positionDistance + colorDistance * colorDistance);

                                if(distance < minDistance)
                                {
                                    minDistance=distance;
                                    clusterInx=i;
                                }
                            }
                        
                        }
                    }

                    clusterMapping[clusterInx].push_back(inx);
                }
            }

            totalColorDiff=0;
            for(int j=0; j<clusterMapping.size(); j++)
            {
                ClusterData average = {
                    glm::vec3(0), glm::vec2(0)
                };
                float inverseSize = 1.0f / (float)clusterMapping[j].size();
                for(int i=0; i<clusterMapping[j].size(); i++)
                {
                    int inx = clusterMapping[j][i];
                    glm::vec2 position(
                        inx % width,
                        inx / width
                    );
                    glm::vec3 color = inputData[inx];

                    average.color += color * inverseSize;
                    average.position += position * inverseSize;
                }

                float colorDiff = glm::distance2(clusterPositions[j].color, average.color);
                totalColorDiff += colorDiff;
                float positionDiff = glm::distance2(clusterPositions[j].position, average.position) / (float)clusterDiagSize;
                totalPositionDiff += positionDiff;

                clusterPositions[j] = average;
            }
        }
        
        for(int j=0; j<clusterMapping.size(); j++)
        {
            ClusterData clusterPos = clusterPositions[j];
            
            glm::vec3 clusterColor(0);
            if(outputMode == OutputMode::ClusterColor)
            {
                clusterColor = clusterPos.color;
            }
            else if(outputMode == OutputMode::RandomColor)
            {
             clusterColor = glm::vec3(
                    (float)rand() / (float)RAND_MAX,
                    (float)rand() / (float)RAND_MAX,
                    (float)rand() / (float)RAND_MAX
                );
            }
            else if(outputMode == OutputMode::GrayScale)
            {
                float grayScale = (float)j / (float)numClusters;
                clusterColor = glm::vec3(grayScale);
            }
            for(int i=0; i<clusterMapping[j].size(); i++)
            {
                inputData[clusterMapping[j][i]] = glm::vec4(clusterColor,1);
            }
        }

		shouldProcess = false;
    }



    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
//
//

//
//------------------------------------------------------------------------
HoughTransform::HoughTransform(bool enabled) : ImageProcess("HoughTransform", "", enabled)
{
    cannyEdgeDetector = new CannyEdgeDetector(true);
    cannyEdgeDetector->size=3;
    cannyEdgeDetector->sigma=1;
    cannyEdgeDetector->threshold=0.038f;
}

void HoughTransform::Unload()
{
    glDeleteProgram(shader);
    cannyEdgeDetector->Unload();
    delete cannyEdgeDetector;
}


void HoughTransform::SetUniforms()
{
}

bool HoughTransform::RenderGui()
{
    bool changed=false;
    
    ImGui::Text("Canny Edge Detector");
    cannyChanged = cannyEdgeDetector->RenderGui();

    changed |= cannyChanged;

    ImGui::Separator();
    ImGui::Text("Edge Linking");

    changed |= ImGui::DragInt("Hough Space Width", &houghSpaceSize, 1, 1, 8192);

    changed |= ImGui::DragInt("Hood Size", &hoodSize, 1, 1, houghSpaceSize);
    
    changed |= ImGui::Checkbox("Add To Image", &addToImage);
    changed |= ImGui::Checkbox("View edges", &viewEdges);

    ImGui::Image((ImTextureID)houghTexture.glTex, ImVec2(256, 128));

    ImGui::DragFloat("Threshold", &threshold, 1, 0, 10000);

    if(ImGui::Button("Process")) shouldProcess=true;
    changed |= shouldProcess;
    return changed;
}


void HoughTransform::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    if(shouldProcess)
    {
        //Recreate textures if size changed
        if(houghTexture.width != houghSpaceSize || houghTexture.height != houghSpaceSize || !houghTexture.loaded)
        {
            TextureCreateInfo tci = {};
            tci.generateMipmaps =false;
            tci.srgb=true;
            tci.minFilter = GL_LINEAR;
            tci.magFilter = GL_LINEAR;     
            if(houghTexture.loaded) houghTexture.Unload();   
            houghTexture = GL_TextureFloat(houghSpaceSize, houghSpaceSize, tci);        
        }
        
        //Only recompute canny when its params changed.
        cannyEdgeDetector->Process(textureIn, textureOut, width, height);

        //Reinitialize the hough space map
        houghSpace.resize(houghSpaceSize * houghSpaceSize);
        std::fill(houghSpace.begin(), houghSpace.end(), glm::vec4(0,0,0,1));

        //Read back edges
        edgeData.resize(width * height, glm::vec4(0));
        glBindTexture(GL_TEXTURE_2D, textureOut);
        glGetTexImage (GL_TEXTURE_2D,
                        0,
                        GL_RGBA, // GL will convert to this format
                        GL_FLOAT,   // Using this data type per-pixel
                        edgeData.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        //Read back color data
        inputData.resize(width * height, glm::vec4(0));
        glBindTexture(GL_TEXTURE_2D, textureIn);
        glGetTexImage (GL_TEXTURE_2D,
                        0,
                        GL_RGBA, // GL will convert to this format
                        GL_FLOAT,   // Using this data type per-pixel
                        inputData.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        //Initialize output
        linesData.resize(width * height);
        std::fill(linesData.begin(), linesData.end(), glm::vec4(0));

        //Build the hough space map
        int diagLength = (int)std::sqrt(width*width + height*height);
        int doubleDiagLength = diagLength*2;
        glm::ivec2 pixelCoord;
        for(pixelCoord.y=0; pixelCoord.y<height; pixelCoord.y++)
        {
            for(pixelCoord.x=0; pixelCoord.x<width; pixelCoord.x++)
            {
                int inx = pixelCoord.y * width + pixelCoord.x;
                float pixelEdge = edgeData[inx].x;
                glm::vec4 pixelColor = inputData[inx];
                
                if(addToImage) 
                {
                    if(viewEdges) linesData[inx] = edgeData[inx];
                    else linesData[inx] = pixelColor;
                }

                //If we're on an edge
                if(pixelEdge>0)
                {
                    for(int t=0; t<houghSpaceSize; t++) //For all angles
                    {
                        glm::vec2 cartesianCoord = glm::vec2(pixelCoord) - glm::vec2(width, height) * 0.5f;
                        float theta = (((float)t / (float)houghSpaceSize) * PI);
                        float rho = (float)cartesianCoord.x * cos(theta) + (float)cartesianCoord.y * sin(theta) + diagLength;
                        float houghSpaceRho = (rho / (float)(doubleDiagLength)) * houghSpaceSize;
                        int r = (int)std::floor(houghSpaceRho);
                        //Increment
                        houghSpace[r * houghSpaceSize + t].x+=1;
                    }
                }
            }    
        }

        //Find peaks

        //Build a peak map that contains the number of votes and the corresponding line
        int peakWidth = houghSpaceSize / hoodSize;
        int peakHeight = houghSpaceSize / hoodSize;
        struct Line 
        {
            float numVotes=0;
            glm::ivec2 x0, x1;
            glm::ivec2 pixel;
        };
        std::vector<Line> peaksMap(peakWidth*peakHeight, {0, glm::ivec2(0,0), glm::ivec2(0,0), glm::ivec2(0)});
                
        float maxVotes=0;
        for(int r=0; r<houghSpaceSize; r++)
        {
            for(int t=0; t<houghSpaceSize; t++)
            {
                float numVotes=houghSpace[r * houghSpaceSize + t].x;
                maxVotes = std::max(maxVotes, numVotes);
                if(numVotes > threshold) //If non empty
                {
                    //Find in which bin this is
                    int peakX = (int)((float)r/(float)houghSpaceSize * (float)(peakHeight-1));
                    int peakY = (int)((float)t/(float)houghSpaceSize * (float)(peakWidth-1));
                    int peakInx = std::min((int)peaksMap.size()-1, std::max(0, peakY * peakWidth + peakX));


                    if(numVotes > peaksMap[peakInx].numVotes) //If the current line has more votes than the one currently in the bin, update it
                    {
                        //Find back angle and distance
                        float rho = ((float)r / (float)houghSpaceSize) * doubleDiagLength - diagLength;
                        float theta = (((float)t / (float)houghSpaceSize) * PI);

                        //Find point on the line
                        float a = cos(theta);
                        float b = sin(theta);
                        float x = (a * rho);
                        float y = (b * rho);

                        //Find the 2 points next to each others on the line
                        glm::vec2 x1(
                            x - b,
                            y + a
                        );
                        glm::vec2 x2(
                            x + b,
                            y - a
                        );
                        
                        //Normalized direction from x1 to x2
                        glm::vec2 direction = glm::normalize(x2-x1);
                        
                        //line endpoints
                        x2 = x1 + direction * 10000;
                        x1 = x1 - direction * 10000;
                        
                        //Cartesian to image space
                        glm::vec2 halfSize(width/2, height/2);
                        x1 += halfSize;
                        x2 += halfSize;                        
                        peaksMap[peakInx]=
                        {
                            numVotes, x1, x2, glm::ivec2(t, r)
                        };
                    }
                }
            }
        }
        
        //Find all the lines in each peak bin
        for(int i=0; i<peaksMap.size(); i++)
        {
            if(peaksMap[i].numVotes>0)
            {
                DrawLine(peaksMap[i].x0, peaksMap[i].x1, linesData, width, height, glm::vec4(1,0,0,1));
            }
        }
        
        //Normalize the hough texture for visualization
        for(int i=0; i<houghSpace.size(); i++)
        {
            houghSpace[i].x /= maxVotes;
        }
        

        glBindTexture(GL_TEXTURE_2D, houghTexture.glTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, houghSpaceSize, houghSpaceSize, 0, GL_RGBA, GL_FLOAT, houghSpace.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, textureOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, linesData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    shouldProcess=false;
}
//
//

//
//------------------------------------------------------------------------
PolygonFitting::PolygonFitting(bool enabled) : ImageProcess("PolygonFitting", "", enabled)
{
}

void PolygonFitting::SetUniforms()
{
}

bool PolygonFitting::RenderGui()
{
    bool changed=false;
    return changed;
}

int PolygonFitting::GetStartIndex(glm::ivec2 b, glm::ivec2 c)
{
    glm::ivec2 diff = c - b;
    if(diff == glm::ivec2(-1, 0)) return 0; //0:Left 
    if(diff == glm::ivec2(-1,-1)) return 1; //1:top left
    if(diff == glm::ivec2( 0,-1)) return 2; //2:top
    if(diff == glm::ivec2( 1,-1)) return 3; //3:top right
    if(diff == glm::ivec2( 1, 0)) return 4; //4:right
    if(diff == glm::ivec2( 1, 1)) return 5; //5:bottom right
    if(diff == glm::ivec2( 0, 1)) return 6; //6:bottom
    if(diff == glm::ivec2(-1, 1)) return 7; //7:bottom left
    return 0;
}

void PolygonFitting::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    //Read back color data
    inputData.resize(width * height, glm::vec4(0));
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

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
    std::vector<point> points;

    //Find the first point
	bool shouldBreak = false;
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            if(inputData[y * width + x].x != 0)
            {
                points.push_back(
                    {
                        glm::ivec2(x, y),
                        glm::ivec2(x-1, y)
                    }
                );
				shouldBreak = true;
				break;
            }
        }
		if (shouldBreak)break;
    }
    
    //Find the ordered list of points
    point *currentPoint = &points[points.size()-1];
    point firstPoint = *currentPoint;
    do {
        glm::ivec2 prevCoord;
        int startInx = GetStartIndex(currentPoint->b, currentPoint->c);
        
        for(int i=startInx; i<startInx + directions.size(); i++)
        {
			int dirInx = i % directions.size();

            glm::ivec2 checkCoord = currentPoint->b + directions[dirInx];
            float checkValue = inputData[checkCoord.y * width + checkCoord.x].x;
            if(checkValue>0)
            {
                point newPoint = 
                {
                    checkCoord,
                    prevCoord,
                };
                points.push_back(newPoint);
				break;
            }
            prevCoord = checkCoord;
        }

        currentPoint = &points[points.size()-1];
    }
    while(currentPoint->b != firstPoint.b && currentPoint->c != firstPoint.c);

    //Remove duplicate point
    points.erase(points.begin() + points.size()-1);

    //Fit a polygon
    int leftMost = 10000000;
    int rightMost = 0;
    int leftMostInx=0;
    int rightMostInx=0;
    for(int i=0; i<points.size(); i++)
    {
        if(points[i].b.x < leftMost)
        {
            leftMost = points[i].b.x;
            leftMostInx=i;
        }
        
        if(points[i].b.x > rightMost)
        {
            rightMost = points[i].b.x;
            rightMostInx=i;
        }
    }

    int A = leftMostInx;
    int B = rightMostInx;
    
    std::stack<int> finalPoints;
    std::stack<int> processPoints;

    finalPoints.push(B);
    processPoints.push(B);
    processPoints.push(A);

    while(true)
    {
        // glm::ivec2 line = points[finalPoints.top()].b - points[processPoints.top()].b;
        glm::ivec2 line0 = points[finalPoints.top()].b;
        glm::ivec2 line1 = points[processPoints.top()].b;
        if(finalPoints.top() < processPoints.top())
        {
            float maxDistance=0;
            int maxInx=0;
            for(int i=finalPoints.top(); i<processPoints.top(); i++)
            {
                float distance = DistancePointLine(line0, line1, points[i].b);
                if(distance > maxDistance)
                {
                    maxDistance=distance;
                    maxInx=i;
                }
            }
            if(maxDistance > threshold)
            {
                processPoints.push(maxInx);
            }
            else
            {
                int lastProcess = processPoints.top();
                finalPoints.push(lastProcess);
                processPoints.pop();
            }
        }
        else
        {
            float maxDistance=0;
            int maxInx=0;
            for(int i=finalPoints.top(); i<points.size(); i++)
            {
                float distance = DistancePointLine(line0, line1, points[i].b);
                if(distance > maxDistance)
                {
                    maxDistance=distance;
                    maxInx=i;
                }
            }
            for(int i=0; i<processPoints.top(); i++)
            {
                float distance = DistancePointLine(line0, line1, points[i].b);
                if(distance > maxDistance)
                {
                    maxDistance=distance;
                    maxInx=i;
                }
            }
            if(maxDistance > threshold)
            {
                processPoints.push(maxInx);
            }
            else
            {
                int lastProcess = processPoints.top();
                finalPoints.push(lastProcess);
                processPoints.pop();
            }
                        
        }
        if(processPoints.size() == 0) break;
    }

    float inverseNumLines = 1.0f / (float)finalPoints.size();
    float color = 0;
    while(finalPoints.size()>1)
    {
        int inx0 = finalPoints.top();
        finalPoints.pop();
        int inx1 = finalPoints.top();
        
        glm::ivec2 a = points[inx0].b;
        glm::ivec2 b = points[inx1].b;
        DrawLine(a, b, inputData, width, height, glm::vec4(color,1,0,1));

        color += inverseNumLines;
    }

    //Calculate the distance with each points between A and B 

    
    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}
//
//
//
//------------------------------------------------------------------------
OtsuThreshold::OtsuThreshold(bool enabled) : ImageProcess("OtsuThreshold", "shaders/Threshold.glsl", enabled)
{
}

void OtsuThreshold::SetUniforms()
{
    glUniform1i(glGetUniformLocation(shader, "global"), 1);
    glUniform3f(glGetUniformLocation(shader, "lowerBound"), 0,0,0);
    glUniform3f(glGetUniformLocation(shader, "upperBound"), threshold,threshold,threshold);
}

bool OtsuThreshold::RenderGui()
{
    bool changed=false;
    return changed;
}


void OtsuThreshold::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    //Read back color data
    inputData.resize(width * height, glm::vec4(0));
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    inputData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    std::vector<float> probabilities(255, 0);
    float globalMean=0;
    float inverseSize = 1.0f / (float)inputData.size();
    for(int i=0; i<inputData.size(); i++)
    {
        float grayScale = (inputData[i].r + inputData[i].g +inputData[i].b) * 0.33333f;
        int grayScaleInt = (int)(grayScale * 255.0f);
        
        probabilities[grayScaleInt]+= inverseSize;
        globalMean += inverseSize * grayScale;
    }
    
    float globalVariance=0;
    for(int i=0; i<inputData.size(); i++)
    {
		float grayScale = (inputData[i].r + inputData[i].g + inputData[i].b) * 0.33333f;
		int grayScaleInt = (int)(grayScale * 255.0f);

        globalVariance += (i - globalMean) * (i - globalMean) * probabilities[grayScaleInt];
    }

//Test
#if 1
    float sum=0;
    for(int i=0; i<probabilities.size(); i++)
    {
        sum += probabilities[i];
    }

#endif


    float maxThreshold=-1;
    float maxVariance=-1;
    for(int j=0; j<255; j++)
    {
		float p1=0;
		float p2=0;
		float m1Mean=0;
		float m2Mean=0;
        float currentThreshold = (float)j / (float)255;

		for(int i=0; i<255; i++)
        {
            if(i<j)
            {
                p1 += probabilities[i];
                m1Mean += i * probabilities[i];
            }
            else
            {
                m2Mean += i * probabilities[i];
            }
        }
		m1Mean /= 255.0f;
		m2Mean /= 255.0f;

        p2 = 1.0f - p1;
		if (p1 > 0 && p2 > 0)
		{
            m1Mean /= (float)p1;
            m2Mean /= (float)p2;

			float variance = p1 * (m1Mean - globalMean) * (m1Mean - globalMean) + p2 * (m2Mean - globalMean) * (m2Mean - globalMean);
			variance /= globalVariance;
			if(variance > maxVariance)
			{
				maxVariance = variance;
				maxThreshold=currentThreshold;
			}
		}
    }
    threshold=maxThreshold;

	glUseProgram(shader);
	SetUniforms();

	glUniform1i(glGetUniformLocation(shader, "textureIn"), 0); //program must be active
    glBindImageTexture(0, textureIn, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	
    glUniform1i(glGetUniformLocation(shader, "textureOut"), 1); //program must be active
    glBindImageTexture(1, textureOut, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
     
    glDispatchCompute((width / 32) + 1, (height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);    
}
//
//


//
//------------------------------------------------------------------------
FFTBlur::FFTBlur(bool enabled) : ImageProcess("FFTBlur", "", enabled)
{}

void FFTBlur::SetUniforms()
{
}

bool FFTBlur::RenderGui()
{
    bool changed=false;

    int filterInt = (int)filter;
    changed |= ImGui::Combo("Filter ", &filterInt, "Box\0Circle\0Gaussian\0\0");
    filter = (Type)filterInt;

    if(filter == Type::GAUSSIAN)
    {
        changed |= ImGui::DragFloat("Sigma", &sigma, 0.01f, 0, 1);
    }
    else
    {
        changed |= ImGui::DragInt("Radius", &radius, 1, 0);
    }
    return changed;
}
void CorrectCoordinates(unsigned int width, unsigned int height, unsigned int i, unsigned int j, unsigned int *x,
        unsigned int *y) {

    if(i < width/2) *x = i + width / 2;
    else            *x = i - width / 2;
    
    if(j < height/2) *y = j + height / 2;
    else             *y = j - height / 2;

}

void change_symmetryBackwards(unsigned int width, unsigned int height, unsigned int i, unsigned int j, unsigned int *x,
        unsigned int *y) {


    if (i < width / 2 && j < height / 2) {
        *x = i - width / 2;
        *y = j - height / 2;
    }
    if (i >= width / 2 && j < height / 2) {
        *x = i + width / 2;
        *y = j - height / 2;
    }
    if (i < width / 2 && j >= height / 2) {
        *x = i - width / 2;
        *y = j + height / 2;
    }
    if (i >= width / 2 && j >= height / 2) {
        *x = i + width / 2;
        *y = j + height / 2;
    }
}

uint32_t nextPowerOf2(uint32_t input)
{
    uint32_t v = input;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

void FFTBlur::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    //Read back from texture
    std::vector<glm::vec4> textureData(width * height);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    textureData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    //The FFT algo only works with power of 2 sizes
    int correctedWidth = (int)nextPowerOf2((uint32_t)width);
    int correctedHeight = (int)nextPowerOf2((uint32_t)height);

    //Resize if needed
    if(textureDataComplexInRed.size() != correctedWidth * correctedHeight)
    {
        
        textureDataComplexInRed.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
        textureDataComplexInGreen.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
        textureDataComplexInBlue.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));

        textureDataComplexOutCorrectedRed.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
        textureDataComplexOutCorrectedGreen.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
        textureDataComplexOutCorrectedBlue.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
        
        textureDataComplexOutRed.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
        textureDataComplexOutGreen.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
        textureDataComplexOutBlue.resize(correctedWidth * correctedHeight, std::complex<double>(0,0));
    }

    //Fill spatial domain data
    for(int y=0; y<correctedHeight; y++)
    {
        for(int x=0; x<correctedWidth; x++)
        {
            int outInx = y * correctedWidth + x;
            
            if(x < width && y < height)
            {
                int inInx = y * width + x;    
                textureDataComplexInRed[outInx] = std::complex<double>((double)textureData[inInx].r, 0);
                textureDataComplexInGreen[outInx] = std::complex<double>((double)textureData[inInx].g, 0);
                textureDataComplexInBlue[outInx] = std::complex<double>((double)textureData[inInx].b, 0);
            }
            else
            {
                textureDataComplexInRed[outInx] = std::complex<double>((double)0, 0);
                textureDataComplexInGreen[outInx] = std::complex<double>((double)0, 0);
                textureDataComplexInBlue[outInx] = std::complex<double>((double)0, 0);
            }
        }
    }
    
	//Execute forward transform
    fftw_plan planRed = fftw_plan_dft_2d(correctedWidth, correctedHeight, (fftw_complex*)textureDataComplexInRed.data(), (fftw_complex*)textureDataComplexOutRed.data(), FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan planGreen = fftw_plan_dft_2d(correctedWidth, correctedHeight, (fftw_complex*)textureDataComplexInGreen.data(), (fftw_complex*)textureDataComplexOutGreen.data(), FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan planBlue = fftw_plan_dft_2d(correctedWidth, correctedHeight, (fftw_complex*)textureDataComplexInBlue.data(), (fftw_complex*)textureDataComplexOutBlue.data(), FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(planRed);
    fftw_execute(planGreen);
    fftw_execute(planBlue);

    //Correct the coordinates
    for(int y=0; y<correctedHeight; y++)
    {
        for(int x=0; x<correctedWidth; x++)
        {
            int flatInx = y * correctedWidth + x;
            
            uint32_t correctedX=x, correctedY=y;
            CorrectCoordinates(correctedWidth, correctedHeight, x, y, &correctedX, &correctedY);
            int correctedInx = correctedY * correctedWidth + correctedX;
            
            
            textureDataComplexOutCorrectedRed[flatInx] = textureDataComplexOutRed[correctedInx];
            textureDataComplexOutCorrectedGreen[flatInx] = textureDataComplexOutGreen[correctedInx];
            textureDataComplexOutCorrectedBlue[flatInx] = textureDataComplexOutBlue[correctedInx];
        }
    }

    //Filter
    if(filter == Type::BOX)
    {
        ///Box filter
        int midPointX = correctedWidth/2;
        int midPointY = correctedHeight/2;
        for(int y=0; y<correctedHeight; y++)
        {
            for(int x=0; x<correctedWidth; x++)
            {
                if(x < midPointX -radius || x > midPointX + radius || y < midPointY - radius || y > midPointY + radius)
                {
                    int inx = y * correctedWidth + x;
                    textureDataComplexOutCorrectedRed[inx]= std::complex<double>(0,0);
                    textureDataComplexOutCorrectedGreen[inx]= std::complex<double>(0,0);
                    textureDataComplexOutCorrectedBlue[inx]= std::complex<double>(0,0);
                }
                else
                {
                }
            }
        }
    }
    else if(filter == Type::CIRCLE)
    {
        for(int y=0; y<correctedHeight; y++)
        {
            for(int x=0; x<correctedWidth; x++)
            {
                glm::ivec2 coord = glm::ivec2(x,y) - glm::ivec2(correctedWidth/2, correctedHeight/2);
                if(sqrt(coord.x * coord.x + coord.y * coord.y) > radius)
                {
                    int inx = y * correctedWidth + x;
                    textureDataComplexOutCorrectedRed[inx]= std::complex<double>(0,0);
                    textureDataComplexOutCorrectedGreen[inx]= std::complex<double>(0,0);
                    textureDataComplexOutCorrectedBlue[inx]= std::complex<double>(0,0);
                }
            }
        }
    }
    else if (filter == Type::GAUSSIAN)
    {
        float oneOverSigma = 1.0f / sigma;
        float oneOverSigmaSquared = oneOverSigma * oneOverSigma;
        ///Gaussian
        for(int y=0; y<correctedHeight; y++)
        {
            for(int x=0; x<correctedWidth; x++)
            {
                glm::vec2 coord = glm::vec2(x,y) - glm::vec2(correctedWidth/2, correctedHeight/2);
                float mag = exp(-(coord.x * coord.x + coord.y * coord.y) / (2 * oneOverSigmaSquared));
                // int currendRad =sqrt(coord.x * coord.x + coord.y * coord.y); 
                // if(currendRad > radius)
                // {
                    // float cutoff = std::min((float)currendRad / (float)radius, 1.0f);
                    int inx = y * correctedWidth + x;
                    textureDataComplexOutCorrectedRed[inx] *= (double)mag;
                    textureDataComplexOutCorrectedGreen[inx] *= (double)mag;
                    textureDataComplexOutCorrectedBlue[inx] *= (double)mag;
                // }
            }
        }
    }
    
    //Change cooridnates
    for(int y=0; y<correctedHeight; y++)
    {
        for(int x=0; x<correctedWidth; x++)
        {
            int flatInx = y * correctedWidth + x;

            uint32_t correctedX=x, correctedY=y;
            CorrectCoordinates(correctedWidth, correctedHeight, x, y, &correctedX, &correctedY);
            int correctedInx = correctedY * correctedWidth + correctedX;
            textureDataComplexOutRed[correctedInx] = textureDataComplexOutCorrectedRed[flatInx];
            textureDataComplexOutGreen[correctedInx] = textureDataComplexOutCorrectedGreen[flatInx];
            textureDataComplexOutBlue[correctedInx] = textureDataComplexOutCorrectedBlue[flatInx];
        }
    }
    
    //Execute backwards transform
    fftw_plan backwardsRed = fftw_plan_dft_2d(correctedWidth, correctedHeight, (fftw_complex*)textureDataComplexOutRed.data(), (fftw_complex*)textureDataComplexInRed.data(),FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_plan backwardsGreen = fftw_plan_dft_2d(correctedWidth, correctedHeight, (fftw_complex*)textureDataComplexOutGreen.data(), (fftw_complex*)textureDataComplexInGreen.data(),FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_plan backwardsBlue = fftw_plan_dft_2d(correctedWidth, correctedHeight, (fftw_complex*)textureDataComplexOutBlue.data(), (fftw_complex*)textureDataComplexInBlue.data(),FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(backwardsRed);
    fftw_execute(backwardsGreen);
    fftw_execute(backwardsBlue);

    //Fill data
    for(int y=0; y<height; y++)
    {
    for(int x=0; x<width; x++)
    {
        int i = y * (correctedWidth) + x;
        float red = (float)textureDataComplexInRed[i].real() / (float)(correctedWidth * correctedHeight);
        float green = (float)textureDataComplexInGreen[i].real() / (float)(correctedWidth * correctedHeight);
        float blue = (float)textureDataComplexInBlue[i].real() / (float)(correctedWidth * correctedHeight);
    
        int outInx = (y * width + x);

#if 0
        float mag = log2((float)abs(textureDataComplexOutCorrectedRed[i]) / (float)correctedWidth + 1);
        // float mag = textureDataComplexOutCorrectedRed[i].real();
        textureData[outInx].r = mag;
        textureData[outInx].g = mag;
        textureData[outInx].b = mag;
        textureData[outInx].a = 1;
#else
        textureData[outInx].r = red;
        textureData[outInx].g = green;
        textureData[outInx].b = blue;
        textureData[outInx].a = 1;
#endif
    }
    }


    fftw_destroy_plan(planRed);
    fftw_destroy_plan(planGreen);
    fftw_destroy_plan(planBlue);
    fftw_destroy_plan(backwardsRed);
    fftw_destroy_plan(backwardsGreen);
    fftw_destroy_plan(backwardsBlue);
    // fftw_destroy_plan(q);



    glBindTexture(GL_TEXTURE_2D, textureOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, textureData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

}

//


ImageLab::ImageLab() {
}


void ImageLab::Load() {
    MeshShader = GL_Shader("shaders/ImageLab/Filter.vert", "", "shaders/ImageLab/Filter.frag");
    Quad = GetQuad();

    // imageProcessStack.AddProcess(new ColorDistance(true));
    // imageProcessStack.AddProcess(new AddImage(true, "D:\\Boulot\\2022\\Lab\\resources\\Gradient.jpg"));
    // imageProcessStack.AddProcess(new AddImage(true, "resources\\StripesHQ.png"));
    // imageProcessStack.AddProcess(new AddImage(true, "resources\\Stripes.png"));
    // imageProcessStack.AddProcess(new AddImage(true, "resources\\StripesHoriz.png"));
    imageProcessStack.AddProcess(new AddImage(true, "resources\\Beach.jpg"));
    // imageProcessStack.AddProcess(new AddImage(true, "resources/peppers.png"));
    imageProcessStack.AddProcess(new SeamCarvingResize(true));
    // imageProcessStack.AddProcess(new Dithering(true));
    // imageProcessStack.AddProcess(new FFTBlur(true));
    // imageProcessStack.AddProcess(new RegionProperties(true));
    // imageProcessStack.AddProcess(new Threshold(true));
    // imageProcessStack.AddProcess(new SmoothingFilter(true));
    // imageProcessStack.AddProcess(new CurveGrading(true));
    // imageProcessStack.AddProcess(new AddNoise(true));
    // imageProcessStack.AddProcess(new MinMaxFilter(true));
    // imageProcessStack.AddProcess(new Equalize(true));
    // imageProcessStack.AddProcess(new FFT(true));
    
}

void ImageLab::Process()
{

}

void ImageLab::RenderGUI() {
    ImGuiIO &io = ImGui::GetIO();
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::Begin("ImageLab", nullptr, window_flags);
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGuiID dockspace_id = ImGui::GetID("ImageLabDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    ////////////////////////////////////////////////////////////////////////////////////
    //Processes side panel
    ////////////////////////////////////////////////////////////////////////////////////
    
    ImGui::Begin("Processes : "); 
    
    if(ImGui::SliderInt("ResolutionX", &imageProcessStack.width, 1, 2048))
    {
        imageProcessStack.Resize(imageProcessStack.width, imageProcessStack.height);
        shouldProcess=true;
    }
    if(ImGui::SliderInt("ResolutionY", &imageProcessStack.height, 1, 2048))
    {
        imageProcessStack.Resize(imageProcessStack.width, imageProcessStack.height);
        shouldProcess=true;
    }

    shouldProcess |= imageProcessStack.RenderGUI();
    ImGui::End();    

    if(shouldProcess || firstFrame)
    {
        std::cout << "PROCESSING "<< std::endl;
        outTexture = imageProcessStack.Process();
        shouldProcess=false;
    }
    
   
    ////////////////////////////////////////////////////////////////////////////////////
    //Output central window
    ////////////////////////////////////////////////////////////////////////////////////
    ImGui::Begin("Output");
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    imageProcessStack.outputGuiStart.x = cursor.x + ImGui::GetScrollX();
    imageProcessStack.outputGuiStart.y = cursor.y + ImGui::GetScrollY();

    //Calculate mouse position in this space
    glm::vec2 outputWindowMousePos(io.MousePos.x, io.MousePos.y);
    outputWindowMousePos -= imageProcessStack.outputGuiStart - glm::vec2(ImGui::GetScrollX(), ImGui::GetScrollY());
    if(outputWindowMousePos.x < 0 || 
      outputWindowMousePos.x >=imageProcessStack.width * imageProcessStack.zoomLevel || 
      outputWindowMousePos.y <0 || 
      outputWindowMousePos.y >=imageProcessStack.height * imageProcessStack.zoomLevel ||
      io.MousePos.x < imageProcessStack.outputGuiStart.x
      )
    {
        outputWindowMousePos = glm::vec2(-1);
    }
    else
    {
        outputWindowMousePos = glm::clamp(outputWindowMousePos, glm::vec2(0), imageProcessStack.zoomLevel * glm::vec2(imageProcessStack.width, imageProcessStack.height));
        outputWindowMousePos /= imageProcessStack.zoomLevel;
    }


    ImVec2 texSize((float)imageProcessStack.width * imageProcessStack.zoomLevel, (float)imageProcessStack.height * imageProcessStack.zoomLevel);
    ImGui::Image((ImTextureID)outTexture, texSize);
    for(int i=0; i<imageProcessStack.imageProcesses.size(); i++)
    {
        imageProcessStack.imageProcesses[i]->RenderOutputGui();

        shouldProcess |= imageProcessStack.imageProcesses[i]->MouseMove(outputWindowMousePos.x, outputWindowMousePos.y);
        if(io.MouseClicked[0])  shouldProcess |= imageProcessStack.imageProcesses[i]->MousePressed();
        if(io.MouseReleased[0]) shouldProcess |= imageProcessStack.imageProcesses[i]->MouseReleased();
    }


    ImGui::End();

    ////////////////////////////////////////////////////////////////////////////////////
    //Pixel information
    ////////////////////////////////////////////////////////////////////////////////////
    ImGui::Begin("Pixel infos");
    glm::vec4 color(0);

    int inx = (int)(outputWindowMousePos.y * imageProcessStack.width + outputWindowMousePos.x);
    if(inx>=0 && inx < imageProcessStack.outputImage.size())color  = imageProcessStack.outputImage[inx];    

    ImGui::Text("Mouse Position : %f, %f", outputWindowMousePos.x, outputWindowMousePos.y);
    ImGui::SameLine();
    ImGui::Text("Color : %f, %f, %f, %f", color.x, color.y, color.z, color.w);
    ImGui::SameLine();
    ImGui::ColorButton("C", ImVec4(color.r, color.g, color.b, color.a));

    ImGui::SameLine();
    ImGui::DragFloat("Zoom", &imageProcessStack.zoomLevel, 0.01f, 0.1f, 100000);

    ImGui::End();
    
    ImGui::End();    
}

void ImageLab::Render() {



    firstFrame=false;
}

void ImageLab::Unload() {
    Quad->Unload();
    delete Quad;

    MeshShader.Unload();

    imageProcessStack.Unload();
}


void ImageLab::MouseMove(float x, float y) {
}

void ImageLab::LeftClickDown() {
}

void ImageLab::LeftClickUp() {
}

void ImageLab::RightClickDown() {
}

void ImageLab::RightClickUp() {
}

void ImageLab::Scroll(float offset) {
}
