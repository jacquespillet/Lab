#pragma once
#include "../Demo.hpp"
#include <array>


template<class T>
class olcNoiseMaker;

double GetWave(double phase, int waveType, double time, double LFOAmplitude=0, double LFOFrequency=0);
double HertzToRadians(double hertz);

struct Envelope
{
    double phase=0;
    double frequency = 0;
    double attack;
    double decay;
    double release;

    double amplitude;
    double startAmplitude;

    double triggerOnTime;
    double triggerOffTime;
    bool noteDown;

    Envelope()
    {
        attack = 0.01f;
        decay = 1.0f;
        startAmplitude = 1.0;
        amplitude = 0.0;
        release = 1;
        triggerOnTime = 0.0;
        triggerOffTime = 0.0;
        noteDown=false;
    }

    void Press(double time)
    {
        triggerOnTime=time;
        noteDown=true;
    }

    void Release(double time)
    {
        triggerOffTime = time;
        noteDown=false;
    }

    double GetAmplitude(double time)
    {
        double result = 0.0;
        double lifeTime = time - triggerOnTime;

        if(noteDown)
        {
            //Attack
            if(lifeTime <= attack)
            {
                result = (lifeTime / attack) * startAmplitude;
            }

            //Decay
            if(lifeTime > attack && lifeTime <= (attack + decay))
            {
                result = ((lifeTime - attack) / decay) * (amplitude - startAmplitude) + startAmplitude;
            }

            //Sustain
            if(lifeTime > (attack + decay))
            {
                result = amplitude;
            }
        }
        else
        {
            //Release
            result = ((time - triggerOffTime) / release) * (0.0 - amplitude) + amplitude;
        }

        if(result <= 0.0001)
        {
            result=0;
        }


        return result;
    }
};

struct Instrument
{
    double volume;
    Envelope envelope;

    virtual double sound(double time, double frequency)=0;
};

struct Bell : public Instrument
{
    Bell()
    {
        envelope = {};
        envelope.attack = 0.01f;
        envelope.decay = 1.0f;
        envelope.startAmplitude = 1.0;
        envelope.amplitude = 0.0;
        envelope.release = 1;
        envelope.triggerOnTime = 0.0;
        envelope.triggerOffTime = 0.0;
    }

    double sound(double time, double frequency)
    {
        double wave = 0;
        wave += 1.0 * GetWave(frequency * 2, 0, time, 0.001, 5);
        wave += 0.5 * GetWave(frequency * 3, 0, time);
        wave += 0.25 * GetWave(frequency * 4, 0, time);

        return wave;
    }
};

struct note
{
    Instrument *instrument;
};

class AudioLab : public Demo {
public : 
    AudioLab();
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
    void Key(int keyCode, int action);

    void Process();

    // float frequency=440.0f;
    int type=0;
    float amplitude = 1.0f;

    double Noise(double t);

    
    std::array<Envelope, 12> envelopes;
    std::array<int, 12> keys = 
    {
       90, // A
       83, // A#
       88, // B
       67, // C
       70, // C#
       86, // D
       71, // D#
       66, // E
       78, // F
       74, // F#
       77, // G
       44 // G#
    };
    

    int sampleRate = 44100;
    double timeStep;

    Instrument * instrument = nullptr;
    // float LFOFrequency=5.0f;
    // float LFOAmplitude = 0.1f;

private:
    olcNoiseMaker<short>* sound;
};