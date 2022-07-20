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
//https://github.com/micknoise/Maximilian/blob/master/src/maximilian.cpp


//TODO
//  add flange effect https://blog.demofox.org/2015/03/16/diy-synth-flange-effect/
//  add delay https://blog.demofox.org/2015/03/17/diy-synth-delay-effect-echo/
//  add reverb https://blog.demofox.org/2015/03/17/diy-synth-multitap-reverb/
//  add convolution reverb https://blog.demofox.org/2015/03/23/diy-synth-convolution-reverb-1d-discrete-convolution-of-audio-samples/
//  Implement other types of wave from maximilian :
//      phasorBetween
//      sinebuf
//      sinebuf4
//      sawn
//  Add maximilian filters
//      lores
//      hires
//      bandpass
//      lopass
//      hipass

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LowPass::RenderGUI()
{
    ImGui::Checkbox("High Pass Filter", &enabled);
    float f_cutoff = (float)cutoff;
    ImGui::SliderFloat("Cutoff", &f_cutoff, 0, 1);
    cutoff = (double)f_cutoff;
}

void HighPass::RenderGUI()
{
    ImGui::Checkbox("High Pass Filter", &enabled);
    float f_cutoff = (float)cutoff;
    ImGui::SliderFloat("Cutoff", &f_cutoff, 0, 1);
    cutoff = (double)f_cutoff;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Graph::Render(ImDrawList* draw_list)
{

    draw_list->AddRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(255, 255, 255, 255));
    ImGui::InvisibleButton("enveloppeCanvas", ImVec2(size.x, size.y));

    for(int i=0; i<points.size()-1; i++)
    {
        glm::vec2 start = RemapCoord(points[i]);
        glm::vec2 end = RemapCoord(points[i+1]);
        draw_list->AddLine(ImVec2(start.x, start.y), ImVec2(end.x, end.y), IM_COL32(255,255,255,255), 0.1f);
    }
}


//Takes normalized coordinates between 0 and 1
glm::vec2 Graph::RemapCoord(glm::vec2 coord)
{
    glm::vec2 result(0,0);
    result.x  = origin.x + coord.x * size.x;
    result.y  = origin.y + size.y - coord.y * size.y;

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Instrument::RenderGui()
{

    ImGui::Begin("Instrument");
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if(ImGui::CollapsingHeader("Instrument"))
    {
        envelope.RenderGui(draw_list);
        
        if(ImGui::TreeNode("Waves"))
        {
            bool graphParamsChanged=false;

            bool numNotesChanged = ImGui::SliderInt("Number of waves", &numNotes, 1, 8);
            if(numNotesChanged) waveParams.resize(numNotes);

            graphParamsChanged |= numNotesChanged;

            ImGui::Separator();
            
            double totalAmplitude=0;
            for(int i=0; i<waveParams.size(); i++)
            {
                ImGui::Text("Wave %d", i); ImGui::SameLine();
                
                ImGui::PushID(i * 2 + 0);
                    graphParamsChanged |= ImGui::Combo("Wave Type", (int*)&waveParams[i].waveType, "Sine\0Cosine\0Saw\0Triangle\0Square\0Noise\0Impulse\0Sawn\0Rect\0\0");
                ImGui::PopID();

                ImGui::PushID(i*2 + 1);
                    graphParamsChanged |= ImGui::DragFloat("Amplitude Modulation", &waveParams[i].amplitudeModulation, 0.1f, 0, 100);
                ImGui::PopID();
                
                ImGui::PushID(i*2 + 2);
                    graphParamsChanged |= ImGui::SliderFloat("Frequency Modulation", &waveParams[i].frequencyModulation, 0, 10);
                ImGui::PopID();

                totalAmplitude += waveParams[i].amplitudeModulation;
                ImGui::Separator();
            }
            graphParamsChanged |= ImGui::DragFloat("ScaleX", &scaleX, 1, 0, 40);

            ImVec2 enveloppeCanvasPos = ImGui::GetCursorScreenPos();
            waveGraph.origin = glm::vec2(enveloppeCanvasPos.x, enveloppeCanvasPos.y);
            waveGraph.size = glm::vec2(256,256);
            if(graphParamsChanged || waveGraph.points.size()==0)
            {
                waveGraph.points.resize(256);
                Note note(0, CalcFrequency(3,scaleX), 0, this);
                double maxAmplitude = (std::max)(envelope.amplitude, envelope.startAmplitude) * totalAmplitude;
                for(int i=0; i<waveGraph.points.size(); i++)
                {
                    float t = (float)i / (float)waveGraph.points.size();
                    double s = ((sound(&note, t * TWO_PI * scaleX)) / maxAmplitude) * 0.5 + 0.5;
                    waveGraph.points[i] = glm::vec2(t, s);
                }
            }
            waveGraph.Render(draw_list);

            ImGui::TreePop();
        }

        if(ImGui::TreeNode("Filters"))
        {
            if(ImGui::Button("Add"))
            {
                filters.push_back(new HighPass());
            }

            for(int i=0; i<filters.size(); i++)
            {
                filters[i]->RenderGUI();
            }

            ImGui::TreePop();
        }

        ImGui::Checkbox("Do Frequency Decay", &doFrequencyDecay);
    }

    ImGui::End();
}

double Instrument::sound(Note* note, double time)
{
    double wave = 0;
    

    double frequencyMultiplier=1;
    double envelopeAmplitude = envelope.GetAmplitude(time, note->startTime, note->endTime, note->finished, &frequencyMultiplier);
    for(int i=0; i<numNotes; i++)
    {
        double frequency = doFrequencyDecay ? note->frequency * ((1.0 - envelope.frequencyDecay) + (frequencyMultiplier*envelope.frequencyDecay)) : note->frequency;
        wave += waveParams[i].amplitudeModulation * envelopeAmplitude * note->oscillators[i].Wave(frequency * waveParams[i].frequencyModulation, waveParams[i].waveType);
    }

    
    for(int i=0; i<filters.size(); i++)
    {
        if(filters[i]->enabled) wave = filters[i]->Apply(wave);
    }
    
    return wave;
}    
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Envelope::RenderGui(ImDrawList* draw_list)
{
    if(ImGui::TreeNode("Enveloppe"))
    {
        float f_attack = (float)attack;
        ImGui::DragFloat("AttacK", &f_attack, 0.001f, 0, 10000.0f);
        attack = f_attack;

        float f_decay = (float)decay;
        ImGui::DragFloat("Decay", &f_decay, 0.001f, 0, 10000.0f);
        decay = f_decay;

        float f_release = (float)release;
        ImGui::DragFloat("Release", &f_release, 0.001f, 0, 10000.0f);
        release = f_release;
        
        float f_startAmplitude = (float)startAmplitude;
        ImGui::DragFloat("Start Amplitude", &f_startAmplitude, 0.001f, 0, 10000.0f);
        startAmplitude = f_startAmplitude;
        
        float f_amplitude = (float)amplitude;
        ImGui::DragFloat("Amplitude", &f_amplitude, 0.001f, 0, 10000.0f);
        amplitude = f_amplitude;
        
        float f_frequencyDecay = (float)frequencyDecay;
        ImGui::DragFloat("Frequency Decay", &f_frequencyDecay, 0.001f, 0, 1.0f);
        frequencyDecay = f_frequencyDecay;

        

        float totalDuration = f_attack + f_decay + 4.0f + f_release;
        float maxAmplitude = (float)(std::max)(startAmplitude, amplitude);

        ImVec2 enveloppeCanvasPos = ImGui::GetCursorScreenPos();
        enveloppeGraph.origin = glm::vec2(enveloppeCanvasPos.x, enveloppeCanvasPos.y);
        enveloppeGraph.size = glm::vec2(256,256);

        glm::vec2 attackLineStart(0,0);
        glm::vec2 attackLineEnd = glm::vec2(attack/totalDuration, startAmplitude/maxAmplitude);
        glm::vec2 decayLineEnd = glm::vec2(attackLineEnd.x + (decay/totalDuration), amplitude/maxAmplitude);
        glm::vec2 noteLineEnd = glm::vec2(decayLineEnd.x + (4.0f/totalDuration), amplitude/maxAmplitude);
        glm::vec2 releaseLineEnd(noteLineEnd.x + (release/totalDuration), 0);  

        enveloppeGraph.points = 
        {
            attackLineStart, attackLineEnd, decayLineEnd, noteLineEnd, releaseLineEnd
        };
        
        enveloppeGraph.Render(draw_list);

        ImGui::TreePop();
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double Oscillator::SineWave(double frequency)
{
	double result=sin (phase*(TWO_PI));

	if ( phase >= 1.0 ) phase -= 1.0;
	phase += (1.0/(sampleRate/(frequency)));

	return(result);    
}

double Oscillator::CosineWave(double frequency)
{
	double result=cos (phase*(TWO_PI));

	if ( phase >= 1.0 ) phase -= 1.0;
	phase += (1.0/(sampleRate/(frequency)));

	return(result);   
}
double Oscillator::SawWave(double frequency)
{
    double result=0;
	result=phase;
	if ( phase >= 1.0 ) phase -= 2.0;
	phase += (1.0/(sampleRate/(frequency))) * 2.0;
	return(result);    
}
double Oscillator::TriangleWave(double frequency)
{
    double result=0;
	if ( phase >= 1.0 ) phase -= 1.0;
	phase += (1./(sampleRate/(frequency)));

	if (phase <= 0.5 ) {
		result =(phase - 0.25) * 4;
	} else {
		result =((1.0-phase) - 0.25) * 4;
	}
	return(result);    
}
double Oscillator::SquareWave(double frequency)
{
    double result=0;
	if (phase<0.5) result=-1;
	if (phase>0.5) result=1;
	if ( phase >= 1.0 ) phase -= 1.0;
	phase += (1.0/(sampleRate/(frequency)));
	return(result);  
}
double Oscillator::NoiseWave(double frequency)
{
    double result=0;
	float r = rand()/(float)RAND_MAX;
	result =r*2-1;
	return(result);   
}

double Oscillator::ImpulseWave(double frequency)
{
    if ( phase >= 1.0 ) phase -= 1.0;
    
    double phaseInc = (1./(sampleRate/(frequency)));
    double result = phase < phaseInc ? 1.0 : 0.0;
    phase += phaseInc;
    return result;  
}

double Oscillator::PhasorWave(double frequency)
{
	double result=phase;
	if ( phase >= 1.0 ) phase -= 1.0;
	phase += (1./(sampleRate/(frequency)));
	return(result); 
}
double Oscillator::RectWave(double frequency)
{
    double result=1;
    return result;    
}

double Oscillator::Wave(double frequency, WaveType type)
{
    if(type==WaveType::Sine)
    {
        return SineWave(frequency);
    }
    else if(type == WaveType::Cosine)
    {
        return CosineWave(frequency);
    }
    else if(type == WaveType::Saw)
    {
        return SawWave(frequency);
    }
    else if(type == WaveType::Triangle)
    {
        return TriangleWave(frequency);
    }
    else if(type == WaveType::Square)
    {
        return SquareWave(frequency);
    }
    else if(type == WaveType::Noise)
    {
        return NoiseWave(frequency);
    }
    else if(type == WaveType::Impulse)
    {
        return ImpulseWave(frequency);
    }
    else if(type == WaveType::Noise)
    {
        return NoiseWave(frequency);
    }
    else if(type == WaveType::Phasor)
    {
        return PhasorWave(frequency);
    }
    else if(type == WaveType::Rect)
    {
        return RectWave(frequency);
    }

    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CalcFrequency(float fOctave,float fNote)
{
    return (float)(440*pow(2.0,((double)((fOctave-4)*12+fNote))/12.0));
}

double HertzToRadians(double hertz)
{
    return hertz * 2.0f * PI;
}

double MakeSound(void *objectPointer, double time)
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Clip::Clip()
{
    initialized=true;
}
Clip::Clip(AudioPlayer *audioPlayer, Arrangement* arrangement) : arrangement(arrangement)
{
    piano.note = Note();
    piano.instrument = new Harmonica();
    piano.note.instrument = piano.instrument;
    piano.note.oscillators.resize(piano.instrument->numNotes);
    player = audioPlayer;
    initialized=true;    

    
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
void Clip::RenderPianoView()
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
    glUniform2f(glGetUniformLocation(piano.keysShader, "keysWindowSize"), (float)piano.keysWidth, (float)sequencerHeight);
    
    glDispatchCompute((piano.keysRenderTarget.width / 32) + 1, (piano.keysRenderTarget.height / 32) + 1, 1);
	glUseProgram(0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);    

    ImGuiStyle& style = ImGui::GetStyle();
    float padding = style.WindowPadding.x;
    
    ImGui::Begin("Keys", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
    sequencer.width = ImGui::GetWindowWidth() - piano.keysWidth - ImGui::GetCursorPos().x;
    
    if(ImGui::Button("Play"))
    {
        FillAudioBuffer();
    }


    
    ImGui::PushID(0);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ piano.keysWidth);
    ImGui::DragFloat("", &sequencer.zoomX, 0.1f, 0.1f, 10000, "Zoom (Drag)");
    ImGui::PopID();

    int numCellsVisible = (int)((arrangement->recordDuration / sequencer.zoomX) / sequencer.cellDuration);
    int totalCells = (int)(arrangement->recordDuration / sequencer.cellDuration);
    numCellsVisible = (std::min)(numCellsVisible, totalCells);

    int difference = totalCells - numCellsVisible;

    ImGui::PushID(1);
    ImGui::SetNextItemWidth(sequencer.width);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+ piano.keysWidth);
    ImGui::SliderInt("", &sequencer.startX, 0, difference, "");
    ImGui::PopID();

    ImVec2 keyWindowPos = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)piano.keysRenderTarget.glTex, ImVec2(piano.keysWidth, sequencerHeight));
    ImGui::SameLine();
    

    //Piano mouse pos
    piano.mousePos = glm::vec2(io.MousePos.x - (int)keyWindowPos.x - padding, io.MousePos.y - keyWindowPos.y);
    if(piano.mousePos.x < 0 || piano.mousePos.y < 0 || piano.mousePos.x > piano.keysWidth || piano.mousePos.y > sequencerHeight)
    {
        piano.mousePos = glm::vec2(-1,-1);
    }


    //Sequencer mouse pos
    ImVec2 sequencePos = ImGui::GetCursorScreenPos();
    sequencer.mousePos = glm::vec2(io.MousePos.x - sequencePos.x, io.MousePos.y - sequencePos.y);
    if(sequencer.mousePos.x < 0 || sequencer.mousePos.y < 0 || sequencer.mousePos.x > sequencer.width || sequencer.mousePos.y > sequencerHeight)
    {
        sequencer.mousePos = glm::vec2(-1,-1);
    }

    
    //Set current hovered cell positions
    if(sequencer.mousePos.x>=0)
    {
        float normalizedMousePosX = sequencer.mousePos.x / sequencer.width;
        float normalizedMousePosY = sequencer.mousePos.y / sequencerHeight;

        sequencer.hoveredCellX = (int)std::floor(normalizedMousePosX * (float)numCellsVisible);
        sequencer.hoveredCellY = (int)std::floor(normalizedMousePosY * (float)numNotes);
    }
    else
    {
        sequencer.hoveredCellX=-1;
        sequencer.hoveredCellY=-1;
    }


    
#if 1
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();


    float vertSpacing = sequencer.width/(float)numCellsVisible;
    int cellsPerSecond = (int)(1.0f / sequencer.cellDuration);
    
    //Draw vertical lines and background
    for(int i=0; i<numCellsVisible; i++)
    {
        float y =canvasPos.y;
        float x= canvasPos.x + vertSpacing * (float)i;

        //Background
        {
            int second = (int)std::floor((float)(i + sequencer.startX) * sequencer.cellDuration);
            ImU32 backgroundColor = IM_COL32(120, 120, 120, 255);
            if((second+1)%2==0) backgroundColor = IM_COL32(100, 100, 100, 255);
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + vertSpacing, y + sequencerHeight), backgroundColor);
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
            
            draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + sequencerHeight), color, thickness);
        }   
    }

    //Horizontal lines
    float horizSpacing = sequencerHeight / 12.0f;
    for(int i=0; i<numNotes+1; i++)
    {
        float y =canvasPos.y + (float)i * horizSpacing;
        float x= canvasPos.x;
        draw_list->AddLine(ImVec2(x, y), ImVec2(x + sequencer.width, y), IM_COL32(255, 255, 255, 255), 0.1f);
    }

    

    //Highlight the hovered cell
    if(sequencer.hoveredCellX>=0)
    {
        float startX = canvasPos.x + sequencer.hoveredCellX * vertSpacing;
        float endX   = startX + vertSpacing;
        float startY = canvasPos.y + sequencer.hoveredCellY * horizSpacing;
        float endY   = startY + horizSpacing;
        
        draw_list->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), IM_COL32(255, 255, 0, 255));
    }


    //Draw all the recorded notes.
    //Also handles note resizing in the loop to avoid 2 loops.
    if(!sequencer.resizingNote) sequencer.resizeNoteHash=-1; //If we're not resizing, set the resized note ID to -1
    for (auto& note : recordedNotes)
    {
        float key = note.second.key;
        
        float startTime = (float)note.second.startTime;
        float endTime = (float)note.second.endTime;
        float cellsCovered = (endTime - startTime) / sequencer.cellDuration; //How many cells are covered by the note ?
        
        //In which cell does the note falls, given the start X parameter
        int cellX = (int)std::ceil((startTime / sequencer.cellDuration) - sequencer.startX);

        //Actual pixel coordinates to draw the cell
        float startX = (canvasPos.x + cellX * vertSpacing);
        float overlap = (std::max)(0.0f, canvasPos.x - startX); //When the note is overlapping the left piano keys, we start later, and end sooner
        startX += overlap;
        float endX = startX + vertSpacing * cellsCovered;
        endX -= overlap;

        float startY = canvasPos.y + (key / numNotes) * sequencerHeight;
        float endY = startY + horizSpacing;
        if(cellX < numCellsVisible && endX >= canvasPos.x) 
        {
            draw_list->AddRectFilled(ImVec2(startX, startY), ImVec2(endX, endY), IM_COL32(255, 255, 255, 255));
        }
        

        //if we're not resizing, find a resizing candidate and set the hash
        if(!sequencer.resizingNote)
        {
            //Check if the mouse is at the bounds of the note
            float mouseX = io.MousePos.x;
            bool intersectLeft = mouseX > startX && mouseX < startX + 10; 
            bool intersectRight = mouseX < endX && mouseX > endX - 10;
            if(intersectLeft || intersectRight)
            {
                //Set the cursor to resize
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                
                //Set the note candidate hash and the direction
                int numCells = (int)(arrangement->recordDuration / sequencer.cellDuration);
                float correctedXPosition = sequencer.hoveredCellX + (float)sequencer.startX;
                int noteHash = (int)std::ceil(sequencer.hoveredCellY * numCells + correctedXPosition);
                sequencer.resizeNoteHash = noteHash;
                sequencer.resizeNoteDirection= intersectLeft ? -1.0f : 1.0f;
            }
        }
    }

    //Add note
    if(io.MouseClicked[0])
    {
        piano.currentKey = (int)((piano.mousePos.y / sequencerHeight) * 12.0f);

        if(piano.mousePos.x>0)
        {
            piano.note.frequency = CalcFrequency(3, (float)piano.currentKey);
            piano.note.Press(player->sound->GetTime());
        }

        int numCells = (int)(arrangement->recordDuration / sequencer.cellDuration);
        
        //Check that we're not resizing as well
        if(sequencer.mousePos.x > 0 && sequencer.resizeNoteHash==-1)
        {
            float correctedXPosition = sequencer.hoveredCellX + (float)sequencer.startX;
            float startTime = ((float)(correctedXPosition) / (float)numCells) * arrangement->recordDuration;
            float endTime = startTime + (arrangement->recordDuration / numCells);
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

    //if we click and the note hash is not -1
    if(io.MouseClicked[0] && sequencer.resizeNoteHash >=0)
    {
        sequencer.resizingNote=true;
    }
    
    //If we're resizing
    if(sequencer.resizingNote)
    {
        //ERROR : resizing a note that is not in the list 
        //TODO --> should assert this to debug
		if (recordedNotes.find(sequencer.resizeNoteHash) != recordedNotes.end()) 
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            
            
            float correctedXPosition = sequencer.hoveredCellX + (float)sequencer.startX; //position of the hovered cell, taking startX into account
            float time = ((float)(correctedXPosition) / (float)totalCells) * arrangement->recordDuration; //Time between 0 and recordDuration
            
            //Change the note time
            if(sequencer.resizeNoteDirection > 0)recordedNotes[sequencer.resizeNoteHash].endTime = time;
            else                                 recordedNotes[sequencer.resizeNoteHash].startTime = time;
            
            //If we swapped the bounds while resizing
            if(recordedNotes[sequencer.resizeNoteHash].endTime < recordedNotes[sequencer.resizeNoteHash].startTime)
            {
                std::swap(recordedNotes[sequencer.resizeNoteHash].endTime, recordedNotes[sequencer.resizeNoteHash].startTime);
            }
        }
    }

    //Stop resizing
    if(io.MouseReleased[0] && sequencer.resizingNote) 
    {
        sequencer.resizingNote=false;
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    }

    
    //Draw a line while playing
    if(playing)
    {
        double samplesPerCell = (double)sequencer.cellDuration * player->sampleRate;
        double linePos = (double)(playingSample - sequencer.startX * samplesPerCell)  / (numCellsVisible * samplesPerCell);
        
        float x = canvasPos.x + (float)linePos * sequencer.width;
        float y = canvasPos.y;
        if(x > canvasPos.x)
        {
            draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + sequencerHeight), IM_COL32(200, 200, 200, 255), 2);
        }
    }    

    ImGui::InvisibleButton("##SequencerBtn", ImVec2(sequencer.width, sequencerHeight));
    // std::cout << canvasPos.y << " " << sequencerHeight << std::endl;
#endif
    ImGui::End();
}


void Clip::MousePress()
{

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
        double wave = piano.instrument->sound(&piano.note, time);
        result += wave;
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

    //FILTER


    return result;
}

void Clip::FillAudioBuffer()
{
    soundBuffer.clear();
    soundBuffer.resize((size_t)player->sampleRate * (size_t)arrangement->recordDuration);
    double time=0;
    double deltaTime = 1.0 / player->sampleRate;
    for(size_t i=0; i<soundBuffer.size(); i++)
    {
        double result=0;
        

        // double weight = 1.0 / recordedNotes.size();
        for (auto &note : recordedNotes)
        {
            double wave = note.second.instrument->sound(&note.second, time);
            result += wave;         
        }
        
        soundBuffer[i] = result;
        time += deltaTime;
    }

    playing=true;
    playingSample=0;
}

void Clip::AddToAudioBuffer(std::vector<double> *externalSoundBuffer)
{
    double time=0;
    double deltaTime = 1.0 / player->sampleRate;
    for(size_t i=0; i<externalSoundBuffer->size(); i++)
    {
        double result=0;
        // double weight = 1.0 / recordedNotes.size();
        for (auto &note : recordedNotes)
        {
            double wave = note.second.instrument->sound(&note.second, time);
            result += wave;         
        }
        
        (*externalSoundBuffer)[i] += result;
        time += deltaTime;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double AudioLab::Noise(double time)
{
    
    double result=0;
    double weight = 1.0f / (double)notes.size();
    weight=1;

    //Sound from keyboard notes
    std::vector<int> finishedIndices;
	for(int i = 0; i < notes.size(); i++)
    {
        double wave = notes[i].instrument->sound(&notes[i], time);
        if(notes[i].finished) finishedIndices.push_back(i); 
        result += wave;
    }
    for(int i = (int)finishedIndices.size()-1; i>=0; i--)
    {
        if(finishedIndices[i] < notes.size())
        {
            notes.erase(notes.begin() + finishedIndices[i]);
        }
    }
    
	//Sound from each clip -- not playing them if the overall arrangement is playing
	if (!playing)
	{
		for(int i=0; i<arrangement.clips.size(); i++)
		{
			result += arrangement.clips[i].Sound(time);
		}
	}

    //Sound of the whole arrangement
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


    result *= audioPlayer.amplitude;
    result = (std::min)(1.0, (std::max)(-1.0, result));
    return result;
}

AudioLab::AudioLab() {
     
}

void AudioLab::Load() {
    arrangement.clips.resize(2);
    arrangement.clips[0] = Clip(&audioPlayer, &arrangement);
    arrangement.clips[1] = Clip(&audioPlayer, &arrangement);
    
    std::vector<std::string> devices = olcNoiseMaker<short>::Enumerate();
    audioPlayer.sound = new olcNoiseMaker<short>(devices[0], audioPlayer.sampleRate, 1, 8, 512);
    audioPlayer.timeStep = 1.0 / (double)audioPlayer.sampleRate;

    audioPlayer.sound->SetUserFunction(MakeSound, (void*)this);

    for(int i=0; i<frequencies.size(); i++)
    {
        frequencies[i] = CalcFrequency(3, (float)i);
    }
}




void AudioLab::RenderGUI() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::Begin("AudioLab", nullptr, window_flags);
    ImGui::DragFloat("Amplitude", &audioPlayer.amplitude, 0.01f, 0, 1);
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGuiID dockspace_id = ImGui::GetID("AudioLabDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    ImGui::Begin("Clips");
    ImGui::DragFloat("Duration", &arrangement.recordDuration, 0.1f, 1, 500);
    ImGui::Separator();
    if(ImGui::Button("Play"))
    {
        soundBuffer.clear();
        soundBuffer.resize((size_t)audioPlayer.sampleRate * (size_t)arrangement.recordDuration);
        for(int i=0; i<arrangement.clips.size(); i++)
        {
            arrangement.clips[i].AddToAudioBuffer(&soundBuffer);
        }

        playing=true;
        playingSample=0;
    }
    int clipPreviewHeight = 100;
    ImVec2 regionAvailable = ImGui::GetContentRegionAvail();
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    for(int i=0; i<arrangement.clips.size(); i++)
    {
        std::string name = "Clips" + std::to_string(i);
        if(ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            ImVec2 startPos(cursorPos.x, cursorPos.y);
            ImVec2 endPos(cursorPos.x + regionAvailable.x, cursorPos.y + clipPreviewHeight);
            drawList->AddRectFilled(startPos, endPos, IM_COL32(200,200,200,200));
            ImGui::PushID(i);
                if(ImGui::InvisibleButton("", ImVec2(regionAvailable.x, (float)clipPreviewHeight)))
                {
                    arrangement.currentClip=i;
                }
            ImGui::PopID();
        }
    }
    ImGui::End();

    arrangement.clips[arrangement.currentClip].RenderPianoView();    
    arrangement.clips[arrangement.currentClip].piano.instrument->RenderGui();

    
    ImGui::End();
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
    arrangement.clips[arrangement.currentClip].MousePress();
}

void AudioLab::LeftClickUp() {
    arrangement.clips[arrangement.currentClip].MouseRelease();
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
                    Note(audioPlayer.sound->GetTime(), frequencies[i], keyCode, arrangement.clips[arrangement.currentClip].piano.instrument)
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
