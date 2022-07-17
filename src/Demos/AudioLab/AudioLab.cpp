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
        double wave = instrument->sound(pianoNote.frequency, time);
            
        bool finished=false;
        double envelopeAmplitude = instrument->envelope.GetAmplitude(time, pianoNote.startTime, pianoNote.endTime, finished);
        
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
    keysRenderTarget = GL_TextureFloat(256, 2048, tci);
    keysTexture = GL_Texture("resources/AudioLab/keys.png", tci);

    CreateComputeShader("shaders/AudioLab/keys.glsl", &keysShader);
}

void AudioLab::Process()
{

}

void AudioLab::RenderGUI() {
    ImGui::DragFloat("Amplitude", &amplitude, 0.01f, 0, 1);
    ImGui::Combo("Oscillator", &type, "Sine\0Square\0Triangle\0Saw0\0Saw1\0Random\0\0");

    ImGui::SetNextWindowSize(ImVec2(keysWidth, keysHeight), ImGuiCond_Appearing);
    ImGui::Begin("Keys", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
    ImGui::Image((ImTextureID)keysRenderTarget.glTex, ImVec2(keysWidth, keysHeight));
    ImVec2 windowSize = ImGui::GetWindowSize();
    keysWidth = windowSize.x;
    keysHeight = windowSize.y;

    ImVec2 keyWindowPos = ImGui::GetWindowPos();
    mouseInKeyWindowPos = glm::vec2(mouse.positionX - (int)keyWindowPos.x, mouse.positionY - keyWindowPos.y); 
    



    ImGui::End();
}

void AudioLab::Render() 
{
	glUseProgram(keysShader);
	
    glUniform1i(glGetUniformLocation(keysShader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, keysRenderTarget.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, keysTexture.glTex);
    glUniform1i(glGetUniformLocation(keysShader, "keysTexture"), 0);

    glUniform1i(glGetUniformLocation(keysShader, "mousePressed"), (int)mouse.leftPressed);
    glUniform1i(glGetUniformLocation(keysShader, "pressedKey"), currentKey);
    glUniform2fv(glGetUniformLocation(keysShader, "mousePosition"), 1, glm::value_ptr(mouseInKeyWindowPos));
    glUniform2f(glGetUniformLocation(keysShader, "keysWindowSize"), (float)keysWidth, (float)keysHeight);
    
    glDispatchCompute((keysRenderTarget.width / 32) + 1, (keysRenderTarget.height / 32) + 1, 1);
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
    currentKey = (int)((mouseInKeyWindowPos.y / keysHeight) * 12.0f);

    if(mouseInKeyWindowPos.x>0)
    {
        pianoNote.frequency = CalcFrequency(3, (float)currentKey);
        pianoNote.Press(sound->GetTime());
    }
}

void AudioLab::LeftClickUp() {
    pianoNote.Release(sound->GetTime());
    
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
