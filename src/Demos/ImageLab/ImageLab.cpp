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
    
    // ImGui::InvisibleButton("curve", ImVec2(size.x, size.y));
    
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


    if(changed)
    {
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
            if(x < width && x>=0 && y < height && y >=0) image[x * width + y] = color;
        } else { 
            if(y < width && y>=0 && x < height && x >=0)  image[y * width + x] = color;
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
    tci.minFilter = GL_NEAREST;
    tci.magFilter = GL_NEAREST;
    histogramTexture = GL_TextureFloat(256, 256, tci);

    CreateComputeShader("shaders/Histogram.glsl", &histogramShader);
    CreateComputeShader("shaders/ResetHistogram.glsl", &resetHistogramShader);
    CreateComputeShader("shaders/RenderHistogram.glsl", &renderHistogramShader);
    CreateComputeShader("shaders/ClearTexture.glsl", &clearTextureShader);


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

    return resultTexture;
}

bool ImageProcessStack::RenderGUI()
{
    changed=false;
    
    ImGui::Image((ImTextureID)histogramTexture.glTex, ImVec2(255, 255));
    ImGui::Checkbox("R", &histogramR); ImGui::SameLine(); 
    ImGui::Checkbox("G", &histogramG); ImGui::SameLine();
    ImGui::Checkbox("B", &histogramB); ImGui::SameLine();
    ImGui::Checkbox("Gray", &histogramGray);
    
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

        ImGui::EndPopup();
    }

	ImGui::Separator();
    ImGui::Text("Post Processes");
	ImGui::Separator();

    static ImageProcess *selectedImageProcess=nullptr;
    for (int n = 0; n < imageProcesses.size(); n++)
    {
        ImageProcess *item = imageProcesses[n];
        
        ImGui::PushID(n);
        changed |= ImGui::Checkbox("", &imageProcesses[n]->enabled);
        ImGui::PopID();
        ImGui::SameLine();
        bool isSelected=false;
        if(ImGui::Selectable(item->name.c_str(), &isSelected))
        {
            selectedImageProcess = item;
        }

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
    // for(int i=0; i<histogramGray.size(); i++)
    // {
    //     pr[i] = histogramGray[i] / (float)(texture->width * texture->height);
    // }
    // for(int i=1; i<histogramGray.size(); i++)
    // {
    //     s[i] = (int)std::round(255.0f * pr[i]) + s[i-1];
    // }
    
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
Threshold::Threshold(bool enabled) : ImageProcess("Threshold", "shaders/Threshold.glsl", enabled)
{}

void Threshold::SetUniforms()
{
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
    ImGui::Checkbox("Global", &global);
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
    changed |= ImGui::SliderFloat("Density", &density, 0, 1);
    changed |= ImGui::SliderFloat("Intensity", &intensity, -1, 1);
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
AddImage::AddImage(bool enabled, char newFileName[128]) : ImageProcess("AddImage", "shaders/AddImage.glsl", enabled)
{
    strcpy(this->fileName, newFileName);
    if(strcmp(this->fileName, "")!=0) 
    {
        TextureCreateInfo tci = {};
        tci.generateMipmaps =false;
        tci.minFilter = GL_NEAREST;
        tci.magFilter = GL_NEAREST;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
        
		filenameChanged = false;        
    }
}


void AddImage::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "multiplier"), multiplier);    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.glTex);
    glUniform1i(glGetUniformLocation(shader, "textureToAdd"), 0);
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
        tci.minFilter = GL_NEAREST;
        tci.magFilter = GL_NEAREST;        
        
        if(texture.loaded) texture.Unload();
        texture = GL_TextureFloat(std::string(fileName), tci);
        
		changed=true;
		filenameChanged = false;
    }

    return changed;
}

void AddImage::Unload()
{
    texture.Unload();
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
        tci.minFilter = GL_NEAREST;
        tci.magFilter = GL_NEAREST;        
        
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
        tci.minFilter = GL_NEAREST;
        tci.magFilter = GL_NEAREST;        
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
            tci.minFilter = GL_NEAREST;
            tci.magFilter = GL_NEAREST;     
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
void FFTBlur::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
    //Read back from texture
    std::vector<float> textureData(width * height * 4, 0);
    glBindTexture(GL_TEXTURE_2D, textureIn);
    glGetTexImage (GL_TEXTURE_2D,
                    0,
                    GL_RGBA, // GL will convert to this format
                    GL_FLOAT,   // Using this data type per-pixel
                    textureData.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    std::vector<std::complex<double>> textureDataComplexOutCorrectedRed(width * height);
    std::vector<std::complex<double>> textureDataComplexOutCorrectedGreen(width * height);
    std::vector<std::complex<double>> textureDataComplexOutCorrectedBlue(width * height);
    
    std::vector<std::complex<double>> textureDataComplexOutRed(width * height);
    std::vector<std::complex<double>> textureDataComplexOutGreen(width * height);
    std::vector<std::complex<double>> textureDataComplexOutBlue(width * height);
    
    std::vector<std::complex<double>> textureDataComplexInRed(width * height);
    std::vector<std::complex<double>> textureDataComplexInGreen(width * height);
    std::vector<std::complex<double>> textureDataComplexInBlue(width * height);
    
    for(size_t i=0; i<width * height; i++)
    {
        textureDataComplexInRed[i] = std::complex<double>((double)textureData[i * 4 + 0], 0);
        textureDataComplexInGreen[i] = std::complex<double>((double)textureData[i * 4 + 1], 0);
        textureDataComplexInBlue[i] = std::complex<double>((double)textureData[i * 4 + 2], 0);
    }
    
	
    fftw_plan planRed = fftw_plan_dft_2d(width, height, (fftw_complex*)textureDataComplexInRed.data(), (fftw_complex*)textureDataComplexOutRed.data(), FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan planGreen = fftw_plan_dft_2d(width, height, (fftw_complex*)textureDataComplexInGreen.data(), (fftw_complex*)textureDataComplexOutGreen.data(), FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan planBlue = fftw_plan_dft_2d(width, height, (fftw_complex*)textureDataComplexInBlue.data(), (fftw_complex*)textureDataComplexOutBlue.data(), FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(planRed);
    fftw_execute(planGreen);
    fftw_execute(planBlue);


    for(int y=0; y<height; y++)
    {
    for(int x=0; x<width; x++)
    {
        int flatInx = y * width + x;
        
        uint32_t correctedX=x, correctedY=y;
        CorrectCoordinates(width, height, x, y, &correctedX, &correctedY);
        int correctedInx = correctedY * width + correctedX;
        
        
        textureDataComplexOutCorrectedRed[flatInx] = textureDataComplexOutRed[correctedInx];
        textureDataComplexOutCorrectedGreen[flatInx] = textureDataComplexOutGreen[correctedInx];
        textureDataComplexOutCorrectedBlue[flatInx] = textureDataComplexOutBlue[correctedInx];
    }
    }


    if(filter == Type::BOX)
    {
        ///Box filter
        int midPointX = width/2;
        int midPointY = height/2;
        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                if(x < midPointX -radius || x > midPointX + radius || y < midPointY - radius || y > midPointY + radius)
                {
                    int inx = y * width + x;
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
        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                glm::ivec2 coord = glm::ivec2(x,y) - glm::ivec2(width/2, height/2);
                if(sqrt(coord.x * coord.x + coord.y * coord.y) > radius)
                {
                    int inx = y * width + x;
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
        for(int y=0; y<height; y++)
        {
            for(int x=0; x<width; x++)
            {
                glm::vec2 coord = glm::vec2(x,y) - glm::vec2(width/2, height/2);
                float mag = exp(-(coord.x * coord.x + coord.y * coord.y) / (2 * oneOverSigmaSquared));
                // int currendRad =sqrt(coord.x * coord.x + coord.y * coord.y); 
                // if(currendRad > radius)
                // {
                    // float cutoff = std::min((float)currendRad / (float)radius, 1.0f);
                    int inx = y * width + x;
                    textureDataComplexOutCorrectedRed[inx] *= (double)mag;
                    textureDataComplexOutCorrectedGreen[inx] *= (double)mag;
                    textureDataComplexOutCorrectedBlue[inx] *= (double)mag;
                // }
            }
        }
    }

    
    //Revert back
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            int flatInx = y * width + x;

            uint32_t correctedX=x, correctedY=y;
            CorrectCoordinates(width, height, x, y, &correctedX, &correctedY);
            int correctedInx = correctedY * width + correctedX;
            textureDataComplexOutRed[correctedInx] = textureDataComplexOutCorrectedRed[flatInx];
            textureDataComplexOutGreen[correctedInx] = textureDataComplexOutCorrectedGreen[flatInx];
            textureDataComplexOutBlue[correctedInx] = textureDataComplexOutCorrectedBlue[flatInx];
        }
    }
    
    
    fftw_plan backwardsRed = fftw_plan_dft_2d(width, height, (fftw_complex*)textureDataComplexOutRed.data(), (fftw_complex*)textureDataComplexInRed.data(),FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_plan backwardsGreen = fftw_plan_dft_2d(width, height, (fftw_complex*)textureDataComplexOutGreen.data(), (fftw_complex*)textureDataComplexInGreen.data(),FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_plan backwardsBlue = fftw_plan_dft_2d(width, height, (fftw_complex*)textureDataComplexOutBlue.data(), (fftw_complex*)textureDataComplexInBlue.data(),FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(backwardsRed);
    fftw_execute(backwardsGreen);
    fftw_execute(backwardsBlue);

    for(int y=0; y<height; y++)
    {
    for(int x=0; x<width; x++)
    {
        int i = y * (width) + x;
        float red = (float)textureDataComplexInRed[i].real() / (float)(width * height);
        float green = (float)textureDataComplexInGreen[i].real() / (float)(width * height);
        float blue = (float)textureDataComplexInBlue[i].real() / (float)(width * height);
    
        int outInx = 4 * (y * width + x);

#if 0
        float mag = (float)abs(textureDataComplexOutCorrectedRed[i].real()) / (float)(width);
        textureData[outInx + 0] = mag;
        textureData[outInx + 1] = mag;
        textureData[outInx + 2] = mag;
        textureData[outInx + 3] = 1;
#else
        textureData[outInx + 0] = red;
        textureData[outInx + 1] = green;
        textureData[outInx + 2] = blue;
        textureData[outInx + 3] = 1;
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
    // TextureCreateInfo tci = {};
    // tci.generateMipmaps =false;
    // tci.srgb=true;
    // tci.minFilter = GL_NEAREST;
    // tci.magFilter = GL_NEAREST;
    // texture = GL_TextureFloat("resources/Tom.png", tci);
    // texture = GL_TextureFloat("resources/peppers.png", tci);
    // texture = GL_TextureFloat("resources/Sudoku.jpeg", tci);
    // texture = GL_TextureFloat("resources/shape.png", tci);
    
    // tmpTexture = GL_TextureFloat(texture.width, texture.height, tci);
    Quad = GetQuad();
    


    imageProcessStack.Resize(width, height);
    // imageProcessStack.AddProcess(new ColorDistance(true));
    imageProcessStack.AddProcess(new AddImage(true, "D:\\Boulot\\2022\\Lab\\resources\\Tom.PNG"));
    imageProcessStack.AddProcess(new CurveGrading(true));
    // imageProcessStack.AddProcess(new AddNoise(true));
    // imageProcessStack.AddProcess(new MinMaxFilter(true));
    // imageProcessStack.AddProcess(new Equalize(true));
    // imageProcessStack.AddProcess(new FFT(true));
}

void ImageLab::Process()
{

}

void ImageLab::RenderGUI() {
    shouldProcess=false;
    ImGui::ShowDemoWindow();
    ImGui::Begin("Processes : ", nullptr, ImGuiWindowFlags_NoMove); 

    // ImGui::SameLine(); 
    // if(ImGui::Button("+"))
    // {

    // }

    shouldProcess |= imageProcessStack.RenderGUI();
    ImGui::End();    


}

void ImageLab::Render() {
    
    if(shouldProcess || firstFrame)
    {
        std::cout << "PROCESSING "<< std::endl;
        outTexture = imageProcessStack.Process();
        shouldProcess=false;
    }

    glUseProgram(MeshShader.programShaderObject);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outTexture);
    glUniform1i(glGetUniformLocation(MeshShader.programShaderObject, "textureImage"), 0);

    Quad->RenderShader(MeshShader.programShaderObject);

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
