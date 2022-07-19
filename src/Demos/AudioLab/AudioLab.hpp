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
struct ImDrawList;

double GetWave(double phase, int waveType, double time, double LFOAmplitude=0, double LFOFrequency=0);
double HertzToRadians(double hertz);

float CalcFrequency(float fOctave,float fNote);

enum class WaveType
{
    Sine=0
};

struct Graph
{
    void Render(ImDrawList* draw_list);
    std::vector<glm::vec2> points;

    glm::vec2 RemapCoord(glm::vec2 coord);

    glm::vec2 origin;
    glm::vec2 size;
};

struct Oscillator
{
    Oscillator() 
    {
        phase=0;
        sampleRate=44100;
        time=0;
    }
    double SineWave(double frequency);
    double phase=0;
    double sampleRate = 44100;
    double time=0;
};


struct Envelope
{
    double attack;
    double decay;
    double release;

    double amplitude;
    double startAmplitude;

    double frequencyDecay;

    Graph enveloppeGraph;
   
    Envelope()
    {
        attack = 0.01f;
        decay = 1.0f;
        startAmplitude = 1.0;
        amplitude = 0.0;
        release = 1;
        frequencyDecay=0;
    }



    double GetAmplitude(double time, double startTime, double endTime, bool &finished, double *frequencyMultiplier=nullptr)
    {
        finished=false;

        double result = 0.0;
        double lifeTime = time - startTime;

        bool notePressed = (time < endTime);
        if(frequencyMultiplier!=nullptr) *frequencyMultiplier=1;
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
                if(frequencyMultiplier!=nullptr) *frequencyMultiplier = result;
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

    void RenderGui(ImDrawList* draw_list);
    
    
};

struct Note;

struct Instrument
{
    double volume;
    Envelope envelope;
    int numNotes=1;
    struct WaveParams
    {
        float frequencyModulation=1;
        float amplitudeModulation=1;
        WaveType waveType;
    };
    std::vector<WaveParams> waveParams;

    bool doFrequencyDecay=false;
    
    double sound(Note* note, double time);
    void RenderGui(ImDrawList* draw_list);

    Graph waveGraph;
    float scaleX=35;
};

struct Note
{
    Instrument *instrument;
    double startTime=-1;
    double endTime=-1;
    double frequency;
    float key;

    int keyPressed;

    bool finished=false;
    
    std::vector<Oscillator> oscillators;

    Note(){
        Oscillator osc;
        oscillators.resize(1, osc);
    }
    
    Note(double startTime, double endTime, float key, Instrument *instrument) : frequency(frequency), startTime(startTime), endTime(endTime), key(key), instrument(instrument)
    {
        frequency = CalcFrequency(3, key);
        
        Oscillator osc;
        oscillators.resize(instrument->numNotes, osc);
    }

    Note(double time, double frequency, int keyPressed, Instrument *instrument) : frequency(frequency), keyPressed(keyPressed), endTime(-1), instrument(instrument)
    {
        Press(time);
        Oscillator osc;
        oscillators.resize(instrument->numNotes, osc);
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

        numNotes=1;

        waveParams = {
            {1,1, WaveType::Sine}
        };
    }
};

struct Harmonica : public Instrument
{
    Harmonica()
    {
        envelope = {};
        envelope.attack = 0.002f;
        envelope.decay = 0.1f;
        envelope.startAmplitude = 1.0;
        envelope.amplitude = 1.0;
        envelope.release = 0.2;

        numNotes=2;

        waveParams.resize(2);
        waveParams[0].amplitudeModulation = 1;
        waveParams[0].frequencyModulation = 1;
        
        waveParams[1].amplitudeModulation = 3;
        waveParams[1].frequencyModulation = 0.5;
    }
};

struct Drum : public Instrument
{
    Drum()
    {
        envelope = {};
        envelope.attack = 0.002f;
        envelope.decay = 0.1f;
        envelope.startAmplitude = 1.0;
        envelope.amplitude = 0.0;
        envelope.release = 0.2;
        envelope.frequencyDecay=0.8;

        numNotes=1;
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
    Clip(AudioPlayer *audioPlayer);
    Clip();
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
        
        float cellDuration = 0.125f;
        float recordDuration=5;

        float zoomX = 1;
        int startX = 0;

        bool resizingNote=false;
        int resizeNoteHash=-1;
        float resizeNoteDirection;
    } sequencer;
    
    float sequencerHeight=512;
    float windowHeight;
    int numNotes=12;

    void RenderGUI();
    void FillAudioBuffer();

    void MousePress();
    void MouseRelease();

    double Sound(double time);
    
     // glm::vec2 enveloppeCanvasPos;
    // glm::vec2 envelopeCanvasSize = glm::vec2(256, 256);
    std::unordered_map<uint32_t, Note> recordedNotes;
    std::vector<double> soundBuffer;

    AudioPlayer *player;

    bool playing=false;
    uint64_t playingSample=0;

    bool initialized=false;
    
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

    int currentClip=0;
    std::vector<Clip> clips;
    
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