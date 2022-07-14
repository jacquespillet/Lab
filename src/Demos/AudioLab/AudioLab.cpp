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
/*
    Calculate the frequency of any note!
    frequency = 440×(2^(n/12))
 
    N=0 is A4
    N=1 is A#4
    etc...
 
    notes go like so...
    0  = A  - z
    1  = A# - s
    2  = B  - x
    3  = C  - c
    4  = C# - f
    5  = D  - v
    6  = D# - g
    7  = E  - b
    8  = F  - n
    9  = F# - j
    10 = G  - m
    11 = G# - k
*/
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
    for(int i=0; i<envelopes.size(); i++)
    {
        // double wave = 0;
        // wave += 1.0 * GetWave(envelopes[i].frequency, 1, time, 0.001, 5);
        // wave += 0.5 * GetWave(envelopes[i].frequency * 1.5, 1, time);
        // wave += 0.25 * GetWave(envelopes[i].frequency * 2, 1, time);
        // wave += 0.05 * GetWave(envelopes[i].frequency, 5, time);

        double wave = instrument->sound(envelopes[i].frequency, time);
        double envelopeAmplitude = envelopes[i].GetAmplitude(time);
        
        result += wave * envelopeAmplitude;
    }
    result *= amplitude;
    result = (std::min)(1.0, (std::max)(-1.0, result));
    return result;
}

AudioLab::AudioLab() {
    instrument = new Bell();
    std::vector<std::string> devices = olcNoiseMaker<short>::Enumerate();
    sound = new olcNoiseMaker<short>(devices[0], sampleRate, 1, 8, 512);
    timeStep = 1.0 / (double)sampleRate;

    sound->SetUserFunction(Oscillator, (void*)this);

    for(int i=0; i<envelopes.size(); i++)
    {
        envelopes[i].frequency = CalcFrequency(3, (float)i);
    }
}

void AudioLab::Load() {
}

void AudioLab::Process()
{

}

void AudioLab::RenderGUI() {
    // ImGui::DragFloat("Frequency", &frequency);
    ImGui::DragFloat("Amplitude", &amplitude, 0.01f, 0, 1);
    
    // ImGui::DragFloat("LFOAmplitude", &LFOAmplitude, 0.01f, 0, 1);
    // ImGui::DragFloat("LFOFrequency", &LFOFrequency, 0.01f, 0, 10);

    ImGui::Combo("Oscillator", &type, "Sine\0Square\0Triangle\0Saw0\0Saw1\0Random\0\0");
}

void AudioLab::Render() {
}

void AudioLab::Unload() {
}


void AudioLab::MouseMove(float x, float y) {
}

void AudioLab::LeftClickDown() {
}

void AudioLab::LeftClickUp() {
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
                envelopes[i].Press(sound->GetTime());
            }
            else if(action==0)
            {
                envelopes[i].Release(sound->GetTime());
            }   
        }
    }
}

void AudioLab::Scroll(float offset) {
}
