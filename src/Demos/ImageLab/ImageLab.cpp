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

void ImageProcessStack::Resize(int width, int height)
{
    if(tex0.loaded) tex0.Unload();
    if(tex1.loaded) tex1.Unload();
    
    TextureCreateInfo tci = {};
    tci.minFilter = GL_LINEAR;
    tci.magFilter = GL_LINEAR;
    tex0 = GL_TextureFloat(width, height, tci);
    tex1 = GL_TextureFloat(width, height, tci);
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


}

void ImageProcessStack::AddProcess(ImageProcess* imageProcess)
{
    imageProcess->imageProcessStack = this;
    imageProcesses.push_back(imageProcess);
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

GLuint ImageProcessStack::Process(GLuint textureIn, int width, int height)
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
                                    tex0.glTex,
                                    width, 
                                    height);
        }
        else
        {
            bool pairPass = i % 2 == 0;
            activeProcesses[i]->Process(
                                    pairPass ? tex1.glTex : tex0.glTex, 
                                    pairPass ? tex0.glTex : tex1.glTex, 
                                    width, 
                                    height);
        }
    }

    GLuint resultTexture = (activeProcesses.size() % 2 == 0) ? tex1.glTex : tex0.glTex;
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

void ImageProcessStack::RenderGUI()
{

    ImGui::Image((ImTextureID)histogramTexture.glTex, ImVec2(255, 255));
    ImGui::Checkbox("R", &histogramR); ImGui::SameLine(); 
    ImGui::Checkbox("G", &histogramG); ImGui::SameLine();
    ImGui::Checkbox("B", &histogramB); ImGui::SameLine();
    ImGui::Checkbox("Gray", &histogramGray);
    
	ImGui::Separator();
    ImGui::Text("Post Processes");
	ImGui::Separator();

    static ImageProcess *selectedImageProcess=nullptr;
    for (int n = 0; n < imageProcesses.size(); n++)
    {
        ImageProcess *item = imageProcesses[n];
        
        ImGui::PushID(n);
        ImGui::Checkbox("", &imageProcesses[n]->enabled);
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
        selectedImageProcess->RenderGui();
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

void GrayScaleContrastStretch::RenderGui()
{
    ImGui::SetNextItemWidth(255); ImGui::SliderFloat("lowerBound", &lowerBound, 0, 1);
    ImGui::SetNextItemWidth(255); ImGui::SliderFloat("upperBound", &upperBound, 0, 1);

    ImGui::Begin("Parameters : ");
        
    ImGui::End();    
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

void Equalize::RenderGui()
{
    ImGui::Checkbox("Color", &color);
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

void ColorContrastStretch::RenderGui()
{
    
    ImGui::Begin("Parameters : ");
    
 
    ImGui::Checkbox("Global", &global);

    if(global)
    {
        ImGui::DragFloatRange2("Range", &globalLowerBound, &globalUpperBound, 0.01f, 0, 1);
    }
    else
    {
        ImGui::DragFloatRange2("Range Red", &lowerBound.x, &upperBound.x, 0.01f, 0, 1);
        ImGui::DragFloatRange2("Range Gren", &lowerBound.y, &upperBound.y, 0.01f, 0, 1);
        ImGui::DragFloatRange2("Range Blue", &lowerBound.z, &upperBound.z, 0.01f, 0, 1);
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

void Transform::RenderGui()
{
    ImGui::DragFloat2("Translation", &translation[0], 0.5f);
    ImGui::DragFloat("Rotation", &theta, 0.2f);
    ImGui::DragFloat2("Scale", &scale[0], 0.1f);
    ImGui::DragFloat2("Shear", &shear[0], 0.01f);

    ImGui::Combo("Render Mode", &interpolation, "Nearest\0Bilinear\0Bicubic\0\0");

    ImGui::Checkbox("Flip X", &flipX);
    ImGui::Checkbox("Flip Y", &flipY);
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
AddNoise::AddNoise(bool enabled) : ImageProcess("AddNoise", "shaders/AddNoise.glsl", enabled)
{}

void AddNoise::SetUniforms()
{
    glUniform1f(glGetUniformLocation(shader, "density"), density);
    glUniform1f(glGetUniformLocation(shader, "intensity"), intensity);
    glUniform1i(glGetUniformLocation(shader, "randomColor"), (int)randomColor);
}

void AddNoise::RenderGui()
{
    ImGui::SliderFloat("Density", &density, 0, 1);
    ImGui::SliderFloat("Intensity", &intensity, -1, 1);
    ImGui::Checkbox("Random Color", &randomColor);
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

void SmoothingFilter::RenderGui()
{
    ImGui::SliderInt("Size", &size, 0, 128);
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

void GaussianBlur::RenderGui()
{
    shouldRecalculateKernel |= ImGui::SliderInt("Size", &size, 1, maxSize);
    shouldRecalculateKernel |= ImGui::SliderFloat("Sigma", &sigma, 1, 10);


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
    
    imageProcessStack.Resize(texture.width, texture.height);
    imageProcessStack.AddProcess(new Transform(true));
    imageProcessStack.AddProcess(new AddNoise(true));
    imageProcessStack.AddProcess(new GaussianBlur(true));
}

void ImageLab::Process()
{

}

void ImageLab::RenderGUI() {

    ImGui::Begin("Parameters : ");
    imageProcessStack.RenderGUI();
    ImGui::End();    

}

void ImageLab::Render() {
    
    
    GLuint outTexture = imageProcessStack.Process(texture.glTex,texture.width, texture.height);

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
