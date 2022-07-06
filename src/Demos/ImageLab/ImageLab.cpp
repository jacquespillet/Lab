#include "ImageLab.hpp"

#include "GL/glew.h"
#include <glm/gtx/quaternion.hpp>

#include "GL_Helpers/Util.hpp"
#include <fstream>
#include <sstream>
#include <random>

#include "imgui.h"

ImageProcess::ImageProcess(std::string name, std::string shaderFileName, bool enabled) :name(name), shaderFileName(shaderFileName), enabled(enabled)
{
    CreateComputeShader(shaderFileName, &shader);
}

void ImageProcess::SetUniforms()
{
    
}

void ImageProcess::RenderGui()
{

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

GLuint ImageProcessStack::Process(GLuint textureIn, GLuint textureOut, int width, int height)
{
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
                                    textureIn,
                                    textureOut,
                                    width, 
                                    height);
        }
        else
        {
            bool pairPass = i % 2 == 0;
            activeProcesses[i]->Process(
                                    pairPass ? textureIn : textureOut, 
                                    pairPass ? textureOut : textureIn, 
                                    width, 
                                    height);
        }
    }

    return (activeProcesses.size() % 2 == 0) ? textureIn : textureOut;
}

//
//------------------------------------------------------------------------
ContrastStrech::ContrastStrech(GL_TextureFloat *texture, bool enabled) : ImageProcess("ContrastStrech", "shaders/ContrastStrech.glsl", enabled), texture(texture)
{
    histogramGray = std::vector<float>(255, 0);
}

void ContrastStrech::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "lowerBound"), lowerBound);
    glUniform1f(glGetUniformLocation(shader, "upperBound"), upperBound);
}

void ContrastStrech::RenderGui()
{
    ImGui::SliderFloat("lowerBound", &lowerBound, 0, 1);
    ImGui::SliderFloat("upperBound", &upperBound, 0, 1);

    ImGui::Begin("Parameters : ");
    
    float minValueGray = 1e30f;
    float maxValueGray = -1e30f;
    for(int i=0; i<texture->width * texture->height; i++)
    {
        int pixelIndex = i * 4;
        int grayScale = (int)((texture->data[pixelIndex + 0] + texture->data[pixelIndex + 1] + texture->data[pixelIndex + 2]) * 0.33333f * 255.0f);
        
        histogramGray[grayScale]++;
        minValueGray = std::min(minValueGray, histogramGray[grayScale]);
        maxValueGray = std::max(maxValueGray, histogramGray[grayScale]);
    }
    
    
    ImGui::End();    
}
//

//
//------------------------------------------------------------------------
Equalize::Equalize(GL_TextureFloat *texture, bool enabled) : ImageProcess("Equalize", "shaders/Equalize.glsl", enabled), texture(texture)
{
    histogramR = std::vector<float>(255, 0);
    histogramG = std::vector<float>(255, 0);
    histogramB = std::vector<float>(255, 0);
}

void Equalize::SetUniforms()
{
}

void Equalize::RenderGui()
{

    // ImGui::SliderFloat("lowerBound", &lowerBound, 0, 1);
    // ImGui::SliderFloat("upperBound", &upperBound, 0, 1);

    // ImGui::Begin("Parameters : ");
    
    // float minValueGray = 1e30f;
    // float maxValueGray = -1e30f;
    // for(int i=0; i<texture->width * texture->height; i++)
    // {
    //     int pixelIndex = i * 4;
    //     int grayScale = (int)((texture->data[pixelIndex + 0] + texture->data[pixelIndex + 1] + texture->data[pixelIndex + 2]) * 0.33333f * 255.0f);
        
    //     histogramGray[grayScale]++;
    //     minValueGray = std::min(minValueGray, histogramGray[grayScale]);
    //     maxValueGray = std::max(maxValueGray, histogramGray[grayScale]);
    // }
    
    
    ImGui::End();    
}
//

//
//------------------------------------------------------------------------
ColorContrastStretch::ColorContrastStretch(GL_TextureFloat *texture, bool enabled) : ImageProcess("ColorContrastStretch", "shaders/ColorContrastStrech.glsl", enabled), texture(texture)
{
    histogramR = std::vector<float>(255, 0);
    histogramG = std::vector<float>(255, 0);
    histogramB = std::vector<float>(255, 0);
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

void ColorContrastStretch::RenderGui()
{
    // ImGui::SliderFloat3("lowerBound", &lowerBound[0], 0, 1);
    // ImGui::SliderFloat3("upperBound", &upperBound[0], 0, 1);

    ImGui::Begin("Parameters : ");
    
    float minValueR = 1e30f; float maxValueR = -1e30f;
    float minValueG = 1e30f; float maxValueG = -1e30f;
    float minValueB = 1e30f; float maxValueB = -1e30f;
    for(int i=0; i<texture->width * texture->height; i++)
    {
        int pixelIndex = i * 4;
        int r = (int)(texture->data[pixelIndex + 0] * 255.0f);
        int g = (int)(texture->data[pixelIndex + 1] * 255.0f);
        int b = (int)(texture->data[pixelIndex + 2] * 255.0f);
        
        histogramR[r]++;
        minValueR = std::min(minValueR, histogramR[r]);
        maxValueR = std::max(maxValueR, histogramR[r]);
        
        histogramG[r]++;
        minValueG = std::min(minValueG, histogramG[g]);
        maxValueG = std::max(maxValueG, histogramG[g]);
        
        histogramB[r]++;
        minValueB = std::min(minValueB, histogramB[b]);
        maxValueB = std::max(maxValueB, histogramB[b]);
    }

    ImGui::Checkbox("Global", &global);

    if(global)
    {
        ImGui::PlotHistogram("Histogram Red", histogramR.data(), (int)histogramR.size(), 0, NULL, minValueR, maxValueR, ImVec2(300,80));
        ImGui::PlotHistogram("Histogram Green", histogramG.data(), (int)histogramG.size(), 0, NULL, minValueG, maxValueG, ImVec2(300,80));
        ImGui::PlotHistogram("Histogram Blue", histogramB.data(), (int)histogramB.size(), 0, NULL, minValueB, maxValueB, ImVec2(300,80));
        
        ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Lower Bound", &globalLowerBound, 0, 1); 
        ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Upper Bound", &globalUpperBound, 0, 1);
    }
    else
    {
        ImGui::PlotHistogram("Histogram Red", histogramR.data(), (int)histogramR.size(), 0, NULL, minValueR, maxValueR, ImVec2(300,80));
        ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Lower Bound Red", &lowerBound.x, 0, 1); ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Upper Bound Red", &upperBound.x, 0, 1);
        ImGui::PlotHistogram("Histogram Green", histogramG.data(), (int)histogramG.size(), 0, NULL, minValueG, maxValueG, ImVec2(300,80));
        ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Lower Bound Green", &lowerBound.y, 0, 1); ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Upper Bound Green", &upperBound.y, 0, 1);
        ImGui::PlotHistogram("Histogram Blue", histogramB.data(), (int)histogramB.size(), 0, NULL, minValueB, maxValueB, ImVec2(300,80));
        ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Lower Bound Blue", &lowerBound.z, 0, 1); ImGui::SetNextItemWidth(300); ImGui::SliderFloat("Upper Bound Blue", &upperBound.z, 0, 1);
    }
    
    ImGui::End();    
}
//

//
//------------------------------------------------------------------------
Negative::Negative(bool enabled) : ImageProcess("Negative", "shaders/Negative.glsl", enabled)
{}

void Negative::SetUniforms()
{}

void Negative::RenderGui()
{}
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

void Threshold::RenderGui()
{
    ImGui::Checkbox("Global", &global);
    if(global)
    {
        ImGui::SliderFloat("Lower bound", &globalLower, 0, 1);
        ImGui::SliderFloat("upper bound", &globalUpper, 0, 1);
    }
    else
    {
        ImGui::SliderFloat3("Lower bound", &lower[0], 0, 1);
        ImGui::SliderFloat3("upper bound", &upper[0], 0, 1);
    }
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

void Quantization::RenderGui()
{
    ImGui::SliderInt("Levels", &numLevels, 2, 255);
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

void Resampling::RenderGui()
{
    ImGui::SliderInt("Pixel Size", &pixelSize, 1, 255);
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

void GammaCorrection::RenderGui()
{
    ImGui::SliderFloat("Gamma", &gamma, 0.01f, 5);
}
//



ImageLab::ImageLab() {
}


void ImageLab::Load() {
    MeshShader = GL_Shader("shaders/ImageLab/Filter.vert", "", "shaders/ImageLab/Filter.frag");
    TextureCreateInfo tci = {};
    tci.generateMipmaps =false;
    tci.srgb=true;
    tci.minFilter = GL_LINEAR;
    tci.magFilter = GL_LINEAR;
    texture = GL_TextureFloat("resources/Tom.png", tci);
    tmpTexture = GL_TextureFloat(texture.width, texture.height, tci);
    Quad = GetQuad();
    
    
    imageProcessStack.imageProcesses.push_back(new Equalize(&texture, true));
}

void ImageLab::Process()
{

}

void ImageLab::RenderGUI() {

    

    ImGui::Begin("Parameters : ");

    ImGui::Text("Post Processes");
	ImGui::Separator();

    static ImageProcess *selectedImageProcess=nullptr;
    for (int n = 0; n < imageProcessStack.imageProcesses.size(); n++)
    {
        ImageProcess *item = imageProcessStack.imageProcesses[n];
        
        ImGui::PushID(n);
        ImGui::Checkbox("", &imageProcessStack.imageProcesses[n]->enabled);
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
            if (n_next >= 0 && n_next < imageProcessStack.imageProcesses.size())
            {
                imageProcessStack.imageProcesses[n] = imageProcessStack.imageProcesses[n_next];
                imageProcessStack.imageProcesses[n_next] = item;
                ImGui::ResetMouseDragDelta();
            }
        }

        if (isSelected)
            ImGui::SetItemDefaultFocus();    
    }    

	ImGui::Separator();
    
    if(selectedImageProcess != nullptr)
    {
        selectedImageProcess->RenderGui();
    }

    ImGui::End();    
}

void ImageLab::Render() {
    
    
    GLuint outTexture = imageProcessStack.Process(texture.glTex, tmpTexture.glTex, texture.width, texture.height);

    glUseProgram(MeshShader.programShaderObject);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outTexture);
    glUniform1i(glGetUniformLocation(MeshShader.programShaderObject, "textureImage"), 0);

    Quad->RenderShader(MeshShader.programShaderObject);
}

void ImageLab::Unload() {
    Quad->Unload();
    delete Quad;

    MeshShader.Unload();

    texture.Unload();
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
