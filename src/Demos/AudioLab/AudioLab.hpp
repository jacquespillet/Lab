#pragma once
#include "../Demo.hpp"
#include <array>
#include <vector>
#include <unordered_map>
#include "GL_Helpers/GL_Texture.hpp"
#include "GL_Helpers/GL_Shader.hpp"
#include "GL_Helpers/GL_Mesh.hpp"

template<class T>
class olcNoiseMaker;

double GetWave(double phase, int waveType, double time, double LFOAmplitude=0, double LFOFrequency=0);
double HertzToRadians(double hertz);

float CalcFrequency(float fOctave,float fNote);


struct Envelope
{
    double attack;
    double decay;
    double release;

    double amplitude;
    double startAmplitude;

    Envelope()
    {
        attack = 0.01f;
        decay = 1.0f;
        startAmplitude = 1.0;
        amplitude = 0.0;
        release = 1;
    }



    double GetAmplitude(double time, double startTime, double endTime, bool &finished)
    {
        finished=false;

        double result = 0.0;
        double lifeTime = time - startTime;

        bool notePressed = (time < endTime);

        if(notePressed)
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
            result = ((time - endTime) / release) * (0.0 - amplitude) + amplitude;
            if((time - endTime) / release > 1) finished=true;
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

struct Harmonica : public Instrument
{
    Harmonica()
    {
        envelope = {};
        envelope.attack = 0.2f;
        envelope.decay = 0.1f;
        envelope.startAmplitude = 1.0;
        envelope.amplitude = 1.0;
        envelope.release = 0.2;
    }

    double sound(double time, double frequency)
    {
        double wave = 0;
        wave += 1.0 * GetWave(frequency, 1, time, 0.001, 5);
        wave += 0.5 * GetWave(frequency * 1.5, 1, time);
        wave += 0.25 * GetWave(frequency * 2, 1, time);
        wave += 0.05 * GetWave(frequency, 5, time);

        return wave;
    }
};

struct Note
{
    Instrument *instrument;
    double startTime=-1;
    double endTime=-1;
    double frequency;
    float key;

    int keyPressed;
    
    Note(){}
    
    Note(double startTime, double endTime, float key, Instrument *instrument) : frequency(frequency), startTime(startTime), endTime(endTime), key(key), instrument(instrument)
    {
        frequency = CalcFrequency(3, key);
    }

    Note(double time, double frequency, int keyPressed, Instrument *instrument) : frequency(frequency), keyPressed(keyPressed), endTime(-1), instrument(instrument)
    {
        Press(time);
    }

    void Press(double time)
    {
        startTime = time;
        endTime=DBL_MAX;
    }
    void Release(double time)
    {
        endTime=time;
    }
};

class AudioLab;

struct AudioPlayer
{
    float amplitude = 0.1f;
    int sampleRate = 44100;
    double timeStep;
    olcNoiseMaker<short>* sound;
};

struct Clip
{
    struct {
        GL_TextureFloat keysRenderTarget;
        GL_Texture keysTexture;
        GLint keysShader;
        float keysWidth = 64;
        glm::vec2 mousePos;
        int currentKey;
        
        Note note;
        
        Instrument * instrument = nullptr;
    } piano;

    //Sequencer
    struct
    {
        float width = 512;
        glm::vec2 mousePos;

        int hoveredCellX;
        int hoveredCellY;
        
        float cellDuration = 0.1f;
        float recordDuration=5;

        float zoomX = 1;
        int startX = 0;

        bool resizingNote=false;
        int resizeNoteHash=-1;
        float resizeNoteDirection;
    } sequencer;
    
    float windowHeight=512;
    int numNotes=12;

    void RenderGUI();
    void FillAudioBuffer();

    void MousePress();
    void MouseRelease();

    double Sound(double time);

    std::unordered_map<uint32_t, Note> recordedNotes;
    std::vector<double> soundBuffer;

    AudioPlayer *player;

    bool playing=false;
    uint64_t playingSample=0;
    
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

    struct
    {
        float positionX;
        float positionY;
        bool leftPressed=false;
    } mouse;

    double Noise(double t);

    Clip synthClip;

    
    //Used for keyboard input...
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
    std::array<double, 12> frequencies;
    std::vector<Note> notes;

    void PlaySequencer();

    AudioPlayer audioPlayer;
private:
};