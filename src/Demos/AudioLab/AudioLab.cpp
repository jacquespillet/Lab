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

void Clip::RenderGUI()
{
    ImGuiIO& io = ImGui::GetIO();
    
    //Render piano
	glUseProgram(piano.keysShader);
	
    glUniform1i(glGetUniformLocation(piano.keysShader, "textureOut"), 0); //program must be active
    glBindImageTexture(0, piano.keysRenderTarget.glTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, piano.keysTexture.glTex);
    glUniform1i(glGetUniformLocation(piano.keysShader, "keysTexture"), 0);

    glUniform1i(glGetUniformLocation(piano.keysShader, "mousePressed"), (int)io.MouseDown[0]);
    glUniform1i(glGetUniformLocation(piano.keysShader, "pressedKey"), piano.currentKey);
    glUniform2fv(glGetUniformLocation(piano.keysShader, "mousePosition"), 1, glm::value_ptr(piano.mousePos));
    glUniform2f(glGetUniformLocation(piano.keysShader, "keysWindowSize"), (float)piano.keysWidth, (float)windowHeight);
    
    glDispatchCompute((piano.keysRenderTarget.width / 32) + 1, (piano.keysRenderTarget.height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);    

    ImGuiStyle& style = ImGui::GetStyle();
    float padding = style.WindowPadding.x;
    ImGui::SetNextWindowSize(ImVec2(piano.keysWidth + sequencer.width, windowHeight), ImGuiCond_Appearing);


    ImGui::Begin("Keys", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

    if(ImGui::Button("Play"))
    {
        FillAudioBuffer();
    }

    ImGui::DragFloat("Duration", &sequencer.recordDuration, 0.1f, 1, 500);
    ImGui::Separator();
    
    ImGui::PushID(0);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ piano.keysWidth);
    ImGui::DragFloat("", &sequencer.zoomX, 0.1f, 0.1f, 10000, "Zoom (Drag)");
    ImGui::PopID();

    int numCellsVisible = (int)((sequencer.recordDuration / sequencer.zoomX) / sequencer.cellDuration);
    int totalCells = (int)(sequencer.recordDuration / sequencer.cellDuration);
    numCellsVisible = (std::min)(numCellsVisible, totalCells);

    int difference = totalCells - numCellsVisible;

    ImGui::PushID(1);
    ImGui::SetNextItemWidth(sequencer.width);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ piano.keysWidth);
    ImGui::SliderInt("", &sequencer.startX, 0, difference, "");
    ImGui::PopID();

    ImVec2 keyWindowPos = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)piano.keysRenderTarget.glTex, ImVec2(piano.keysWidth, windowHeight));
    
    ImGui::SameLine();

    //Piano mouse pos
    piano.mousePos = glm::vec2(io.MousePos.x - (int)keyWindowPos.x - padding, io.MousePos.y - keyWindowPos.y);
    if(piano.mousePos.x < 0 || piano.mousePos.y < 0 || piano.mousePos.x > piano.keysWidth || piano.mousePos.y > windowHeight)
    {
        piano.mousePos = glm::vec2(-1,-1);
    }

    //Sequencer mouse pos
    ImVec2 sequencePos = ImGui::GetCursorScreenPos();
    sequencer.mousePos = glm::vec2(io.MousePos.x - sequencePos.x, io.MousePos.y - sequencePos.y);
    if(sequencer.mousePos.x < 0 || sequencer.mousePos.y < 0 || sequencer.mousePos.x > sequencer.width || sequencer.mousePos.y > windowHeight)
    {
        sequencer.mousePos = glm::vec2(-1,-1);
    }
    
    
    
    //Set current cell positions
    if(sequencer.mousePos.x>=0)
    {
        float normalizedMousePosX = sequencer.mousePos.x / sequencer.width;
        float normalizedMousePosY = sequencer.mousePos.y / windowHeight;

        sequencer.hoveredCellX = (int)std::floor(normalizedMousePosX * (float)numCellsVisible);
        sequencer.hoveredCellY = (int)std::floor(normalizedMousePosY * (float)numNotes);
    }
    else
    {
        sequencer.hoveredCellX=-1;
        sequencer.hoveredCellY=-1;
    }


    

    ImDrawList* draw_list = ImGui::GetWindowDrawList();



    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    //Vertical lines
    float vertSpacing = sequencer.width/(float)numCellsVisible;


    int cellsPerSecond = (int)(1.0f / sequencer.cellDuration);
    for(int i=0; i<numCellsVisible; i++)
    {
        float y =canvasPos.y;
        float x= canvasPos.x + vertSpacing * (float)i;

        //Background
        {
            int second = (int)std::floor((float)(i + sequencer.startX) * sequencer.cellDuration);
            ImU32 backgroundColor = IM_COL32(120, 120, 120, 255);
            if((second+1)%2==0) backgroundColor = IM_COL32(100, 100, 100, 255);
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + vertSpacing, y + windowHeight), backgroundColor);
        }

        //Line
        {
            ImU32 color = IM_COL32(150, 150, 150, 255);
            float thickness = 0.1f;
            if((i + sequencer.startX) % cellsPerSecond==0) 
            {
                color = IM_COL32(255, 255, 255, 255);
                thickness = 0.3f;
            }
            
            draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + windowHeight), color, thickness);
        }   
    }

    //Horizontal lines
    float horizSpacing = windowHeight / 12.0f;
    for(int i=0; i<numNotes+1; i++)
    {
        float y =canvasPos.y + (float)i * horizSpacing;
        float x= canvasPos.x;
        draw_list->AddLine(ImVec2(x, y), ImVec2(x + sequencer.width, y), IM_COL32(255, 255, 255, 255), 0.1f);
    }

    


    if(sequencer.hoveredCellX>=0)
    {
        float startX = canvasPos.x + sequencer.hoveredCellX * vertSpacing;
        float endX   = startX + vertSpacing;
        float startY = canvasPos.y + sequencer.hoveredCellY * horizSpacing;
        float endY   = startY + horizSpacing;
        
        draw_list->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), IM_COL32(255, 255, 0, 255));
    }


    for (auto note : recordedNotes)
    {
        float startTime = (float)note.second.startTime;
        float key = note.second.key;
        
        //In which cell does the note falls, given the start X parameter
        int cellX = (int)((startTime / sequencer.cellDuration) - sequencer.startX);

        float startX = canvasPos.x + cellX * vertSpacing;
        float endX = startX + vertSpacing;
        float startY = canvasPos.y + (key / numNotes) * windowHeight;
        float endY = startY + horizSpacing;
        if(cellX < numCellsVisible && startX >= canvasPos.x) 
        {
            draw_list->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), IM_COL32(255, 255, 255, 255));
        }
    }

    
    if(playing)
    {
        double samplesPerCell = (double)sequencer.cellDuration * player->sampleRate;
        double linePos = (double)(playingSample - sequencer.startX * samplesPerCell)  / (numCellsVisible * samplesPerCell);
        
        float x = canvasPos.x + (float)linePos * sequencer.width;
        float y =canvasPos.y;
        if(x > canvasPos.x)
        {
            draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + windowHeight), IM_COL32(200, 200, 200, 255), 2);
        }
    }    
    ImGui::End();
}

void Clip::MousePress()
{
    //Add key
    piano.currentKey = (int)((piano.mousePos.y / windowHeight) * 12.0f);

    if(piano.mousePos.x>0)
    {
        piano.note.frequency = CalcFrequency(3, (float)piano.currentKey);
        piano.note.Press(player->sound->GetTime());
    }

    int numCells = (int)(sequencer.recordDuration / sequencer.cellDuration);
    if(sequencer.mousePos.x > 0)
    {
        float correctedXPosition = sequencer.hoveredCellX + (float)sequencer.startX;
        float startTime = ((float)(correctedXPosition) / (float)numCells) * sequencer.recordDuration;
        float endTime = startTime + (sequencer.recordDuration / numCells);
        float key = ((float)sequencer.hoveredCellY / (float)numNotes) * 12;

        int hash = (int)(sequencer.hoveredCellY * numCells + correctedXPosition);
        if (recordedNotes.find(hash) == recordedNotes.end())
        {
            recordedNotes[hash] = Note(startTime, endTime, key, piano.instrument);
        }
        else
        {
            recordedNotes.erase(hash);
        }
    }
}

void Clip::MouseRelease()
{
    if(piano.mousePos.x>0)
    {
        piano.note.Release(player->sound->GetTime());
    }
    
}

double Clip::Sound(double time)
{
    double result=0;
    //Piano note
    {
        double wave = piano.instrument->sound(piano.note.frequency, time);
            
        bool finished=false;
        double envelopeAmplitude = piano.instrument->envelope.GetAmplitude(time, piano.note.startTime, piano.note.endTime, finished);
        
        result += wave * envelopeAmplitude;
    }
    


    if(playing)
    {
        if(playingSample > soundBuffer.size()-1) 
        {
            playing=false;
            return 0;
        }
        result += soundBuffer[playingSample]; 
        playingSample++;
    }

    return result;
}

double AudioLab::Noise(double time)
{
    
    double result=0;
    double weight = 1.0f / (double)notes.size();

	for(int i = (int)notes.size() - 1; i >= 0; i--)
    {

        double wave = notes[i].instrument->sound(notes[i].frequency, time);
        
        bool finished=false;
        double envelopeAmplitude = notes[i].instrument->envelope.GetAmplitude(time, notes[i].startTime, notes[i].endTime, finished);
        
        result += wave * envelopeAmplitude * weight;
    }
    
    result += synthClip.Sound(time);


    result *= audioPlayer.amplitude;
    result = (std::min)(1.0, (std::max)(-1.0, result));
    return result;
}

AudioLab::AudioLab() {
     
}

void AudioLab::Load() {
    
    synthClip.piano.instrument = new Harmonica();
    synthClip.player = &audioPlayer;
    
    std::vector<std::string> devices = olcNoiseMaker<short>::Enumerate();
    audioPlayer.sound = new olcNoiseMaker<short>(devices[0], audioPlayer.sampleRate, 1, 8, 512);
    audioPlayer.timeStep = 1.0 / (double)audioPlayer.sampleRate;

    audioPlayer.sound->SetUserFunction(Oscillator, (void*)this);

    for(int i=0; i<frequencies.size(); i++)
    {
        frequencies[i] = CalcFrequency(3, (float)i);
    }

    TextureCreateInfo tci = {};
    tci.generateMipmaps =false;
    tci.srgb=true;
    tci.minFilter = GL_LINEAR;
    tci.magFilter = GL_LINEAR;
    tci.flip=false;
    synthClip.piano.keysRenderTarget = GL_TextureFloat(256, 2048, tci);
    synthClip.piano.keysTexture = GL_Texture("resources/AudioLab/keys.png", tci);

    CreateComputeShader("shaders/AudioLab/keys.glsl", &synthClip.piano.keysShader);   
}


void Clip::FillAudioBuffer()
{
    soundBuffer.clear();
    soundBuffer.resize((size_t)player->sampleRate * (size_t)sequencer.recordDuration);
    double time=0;
    double deltaTime = 1.0 / player->sampleRate;
    for(size_t i=0; i<soundBuffer.size(); i++)
    {
        double result=0;
        

        // double weight = 1.0 / recordedNotes.size();
        for (auto note : recordedNotes)
        {
            double wave = note.second.instrument->sound(note.second.frequency, time);
            bool finished=false;
            double envelopeAmplitude = note.second.instrument->envelope.GetAmplitude(time, note.second.startTime, note.second.endTime, finished);
            result += wave * envelopeAmplitude;         
        }
        
        soundBuffer[i] = result;
        time += deltaTime;
    }

    playing=true;
    playingSample=0;
}

void AudioLab::RenderGUI() {
    ImGui::DragFloat("Amplitude", &audioPlayer.amplitude, 0.01f, 0, 1);
    
    synthClip.RenderGUI();

}

void AudioLab::Render() 
{
}

void AudioLab::Unload() {
}


void AudioLab::MouseMove(float x, float y) {
    mouse.positionX = x;
    mouse.positionY = y;
}

void AudioLab::LeftClickDown() {
    synthClip.MousePress();
}

void AudioLab::LeftClickUp() {
    synthClip.MouseRelease();
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
                    Note(audioPlayer.sound->GetTime(), frequencies[i], keyCode, synthClip.piano.instrument)
                );
            }
            else if(action==0)
            {
                for(int k=0; k<notes.size(); k++)
                {
                    if(notes[k].keyPressed==keyCode)
                    {
                        notes[k].Release(audioPlayer.sound->GetTime());
                    }
                }
            } 
        }
    }
}

void AudioLab::Scroll(float offset) {
}
