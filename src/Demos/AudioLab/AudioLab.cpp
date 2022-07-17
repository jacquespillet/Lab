#include "AudioLab.hpp"

#include "GL/glew.h"
#include <glm/gtx/quaternion.hpp>

#include "GL_Helpers/Util.hpp"
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>

#include "imgui.h"
#include "olcNoiseMaker.h"

//https://blog.demofox.org/diy-synthesizer/

float CalcFrequency(float fOctave,float fNote)
{
    return (float)(440*pow(2.0,((double)((fOctave-4)*12+fNote))/12.0));
}

double HertzToRadians(double hertz)
{
    return hertz * 2.0f * PI;
}

double Oscillator(void *objectPointer, double time)
{
    AudioLab *audioLab = (AudioLab*)objectPointer;
    return audioLab->Noise(time);
}

double GetWave(double frequency, int waveType, double time, double LFOAmplitude, double LFOFrequency)
{
    double phase = HertzToRadians(frequency) * time + LFOAmplitude * frequency * sin(HertzToRadians(LFOFrequency) * time );
    double wave =0;
    switch(waveType)
    {
        case 0: //Sine
            wave = sin(phase);
            break;
        case 1: //Square
            wave = sin(phase) > 0 ? 1.0f : -1.0f;
            break;
        case 2: //Triangle
            wave = asin(sin(phase) * 2.0 / PI);
            break;
        case 3: //Triangle
        {
            double output=0;
            for(double s=1.0; s<100.0; s++)
            {
                output += sin(s * phase) / s;
            }
            wave = output * 2.0 * PI;
            break;
        }
        case 4: //Saw
            wave = sin(phase);
            break;
        case 5: //Noise
            wave = 2.0f * ((double)rand() / (double)RAND_MAX) - 1.0f;
            break;
        default:
            wave = 0;
            break;
    }

    return wave;
}

double AudioLab::Noise(double time)
{
    double result=0;

	    double weight = 1.0f / (double)notes.size();

	// if (notes.size() == 0) return 0;
    for(int i = (int)notes.size() - 1; i >= 0; i--)
    {

        double wave = instrument->sound(notes[i].frequency, time);
        
        bool finished=false;
        double envelopeAmplitude = instrument->envelope.GetAmplitude(time, notes[i].startTime, notes[i].endTime, finished);
        
        result += wave * envelopeAmplitude * weight;
    }
    
    //Piano note
    {
        double wave = instrument->sound(piano.note.frequency, time);
            
        bool finished=false;
        double envelopeAmplitude = instrument->envelope.GetAmplitude(time, piano.note.startTime, piano.note.endTime, finished);
        
        result += wave * envelopeAmplitude * weight;
    }
    

    result *= amplitude;
    result = (std::min)(1.0, (std::max)(-1.0, result));
    return result;
}

AudioLab::AudioLab() {
    instrument = new Harmonica();
    std::vector<std::string> devices = olcNoiseMaker<short>::Enumerate();
    sound = new olcNoiseMaker<short>(devices[0], sampleRate, 1, 8, 512);
    timeStep = 1.0 / (double)sampleRate;

    sound->SetUserFunction(Oscillator, (void*)this);

    for(int i=0; i<frequencies.size(); i++)
    {
        frequencies[i] = CalcFrequency(3, (float)i);
    }
}

void AudioLab::Load() {
    TextureCreateInfo tci = {};
    tci.generateMipmaps =false;
    tci.srgb=true;
    tci.minFilter = GL_LINEAR;
    tci.magFilter = GL_LINEAR;
    tci.flip=false;
    piano.keysRenderTarget = GL_TextureFloat(256, 2048, tci);
    piano.keysTexture = GL_Texture("resources/AudioLab/keys.png", tci);

    CreateComputeShader("shaders/AudioLab/keys.glsl", &piano.keysShader);
}

void AudioLab::Process()
{

}

void AudioLab::RenderGUI() {
    ImGui::DragFloat("Amplitude", &amplitude, 0.01f, 0, 1);
    ImGui::Combo("Oscillator", &type, "Sine\0Square\0Triangle\0Saw0\0Saw1\0Random\0\0");

    ImGuiStyle& style = ImGui::GetStyle();
    float padding = style.WindowPadding.x;

    ImGui::SetNextWindowSize(ImVec2(piano.keysWidth + sequencer.width, windowHeight), ImGuiCond_Appearing);

    ImGui::Begin("Keys", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration);
    
    
    ImGui::Image((ImTextureID)piano.keysRenderTarget.glTex, ImVec2(piano.keysWidth, windowHeight));
    
    ImVec2 keyWindowPos = ImGui::GetWindowPos();
    glm::vec2 mouseInWindowPos =glm::vec2(mouse.positionX - (int)keyWindowPos.x - padding, mouse.positionY - keyWindowPos.y);  
    
    //Piano mouse pos
    piano.mousePos = mouseInWindowPos;
    if(piano.mousePos.x < 0 || piano.mousePos.y < 0 || piano.mousePos.x > piano.keysWidth || piano.mousePos.y > windowHeight)
    {
        piano.mousePos = glm::vec2(-1,-1);
    }

    //Sequencer mouse pos
    sequencer.mousePos = mouseInWindowPos - glm::vec2(piano.keysWidth + padding, 0);
    if(sequencer.mousePos.x < 0 || sequencer.mousePos.y < 0 || sequencer.mousePos.x > sequencer.width || sequencer.mousePos.y > windowHeight)
    {
        sequencer.mousePos = glm::vec2(-1,-1);
    }
    
    //Set current cell positions
    if(sequencer.mousePos.x>=0)
    {
        float normalizedMousePosX = sequencer.mousePos.x / sequencer.width;
        float normalizedMousePosY = sequencer.mousePos.y / windowHeight;

        sequencer.currentCellX = (int)std::floor(normalizedMousePosX * (float)sequencer.numCells);
        sequencer.currentCellY = (int)std::floor(normalizedMousePosY * (float)numNotes);
    }
    else
    {
        sequencer.currentCellX=-1;
        sequencer.currentCellY=-1;
    }


    
    //Draw sequencer
    ImGui::SameLine();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    //Horizontal lines
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    float horizSpacing = windowHeight / 12.0f;
    for(int i=0; i<numNotes+1; i++)
    {
        float y =canvasPos.y + (float)i * horizSpacing;
        float x= canvasPos.x;
        draw_list->AddLine(ImVec2(x, y), ImVec2(x + 1000, y), IM_COL32(255, 255, 255, 255), 1);
    }

    //Vertical lines
    float vertSpacing = sequencer.width/(float)sequencer.numCells;
    for(int i=0; i<(int)sequencer.numCells; i++)
    {
        float y =canvasPos.y;
        float x= canvasPos.x + vertSpacing * (float)i;
        draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + windowHeight), IM_COL32(255, 255, 255, 255), 1);
    }

    if(sequencer.currentCellX>=0)
    {
        float startX = canvasPos.x + sequencer.currentCellX * vertSpacing;
        float endX   = startX + vertSpacing;
        float startY = canvasPos.y + sequencer.currentCellY * horizSpacing;
        float endY   = startY + horizSpacing;
        draw_list->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), IM_COL32(255, 255, 0, 255));
    }
    


    ImGui::End();
}

void AudioLab::Render() 
{
	glUseProgram(piano.keysShader);
	
    glUniform1i(glGetUniformLocation(piano.keysShader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, piano.keysRenderTarget.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, piano.keysTexture.glTex);
    glUniform1i(glGetUniformLocation(piano.keysShader, "keysTexture"), 0);

    glUniform1i(glGetUniformLocation(piano.keysShader, "mousePressed"), (int)mouse.leftPressed);
    glUniform1i(glGetUniformLocation(piano.keysShader, "pressedKey"), piano.currentKey);
    glUniform2fv(glGetUniformLocation(piano.keysShader, "mousePosition"), 1, glm::value_ptr(piano.mousePos));
    glUniform2f(glGetUniformLocation(piano.keysShader, "keysWindowSize"), (float)piano.keysWidth, (float)windowHeight);
    
    glDispatchCompute((piano.keysRenderTarget.width / 32) + 1, (piano.keysRenderTarget.height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);    

}

void AudioLab::Unload() {
}


void AudioLab::MouseMove(float x, float y) {
    mouse.positionX = x;
    mouse.positionY = y;
}

void AudioLab::LeftClickDown() {
    mouse.leftPressed=true;

    //Add key
    piano.currentKey = (int)((piano.mousePos.y / windowHeight) * 12.0f);

    if(piano.mousePos.x>0)
    {
        piano.note.frequency = CalcFrequency(3, (float)piano.currentKey);
        piano.note.Press(sound->GetTime());
    }


    
}

void AudioLab::LeftClickUp() {
    if(piano.mousePos.x>0)
    {
        piano.note.Release(sound->GetTime());
    }
    
    mouse.leftPressed=false;
}

void AudioLab::RightClickDown() {
}

void AudioLab::RightClickUp() {
}

void AudioLab::Key(int keyCode, int action)
{
    for(size_t i=0; i<keys.size(); i++)
    {
        if(keyCode == keys[i])
        {
            if(action==1)
            {
                notes.push_back(
                    Note(sound->GetTime(), frequencies[i], keyCode)
                );
            }
            else if(action==0)
            {
                for(int k=0; k<notes.size(); k++)
                {
                    if(notes[k].keyPressed==keyCode)
                    {
                        notes[k].Release(sound->GetTime());
                    }
                }
            } 
        }
    }
}

void AudioLab::Scroll(float offset) {
}
