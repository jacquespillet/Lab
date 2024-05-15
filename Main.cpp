#define STB_IMAGE_IMPLEMENTATION

#include "Common.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>


#include "Demos/Demo.hpp"
#include "Demos/ImageLab/ImageLab.hpp"
#include "Demos/AudioLab/AudioLab.hpp"



struct DemoManager {
    Demo *demo;
    DemoManager(){
        demo=nullptr;
    }
    void ClearDemo() {
        if(demo!=nullptr) {
            demo->Unload();
            delete demo;
        }
    }

    void LoadDemo(int demoInx) {
        switch (demoInx)
        {
        case 0:
            demo = new ImageLab;
            demo->windowWidth = windowWidth;
            demo->windowHeight = windowHeight;
            demo->Load();
            break;
        case 1:
            demo = new AudioLab;
            demo->windowWidth = windowWidth;
            demo->windowHeight = windowHeight;
            demo->Load();
            break;
        default:
            break;
        }
    }

    void MouseMove(float xpos, float ypos ) {
        if(demo!=nullptr) {
            demo->MouseMove(xpos, ypos);
        }
    }

    void MouseAction(int button, int action, int mods) {
        if(demo!=nullptr) {
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
                demo->RightClickDown();
            } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE){
                demo->RightClickUp();
            }
            else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
                demo->LeftClickDown();
            }
            else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
                demo->LeftClickUp();
            }
        }
    }    

    void Scroll(float offset) {
        if(demo!=nullptr) {
            demo->Scroll(offset);
        }
    }

    void Key(int keyCode, int action)
    {
        if(demo !=nullptr)
        {
            demo->Key(keyCode, action);
        }
    }

    void Render() {
        if(demo!=nullptr) {
            demo->Render();
        }
    }

    void RenderGUI()
    {
        if(demo!=nullptr) {
            demo->RenderGUI();
        }
    }

    int windowWidth, windowHeight;

};

static void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}    


void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

struct App {
public:
    App(){}
    int InitWindow() {
        // Setup window
        glfwSetErrorCallback(ErrorCallback);
        if (!glfwInit())
            return 1;

        // GL 3.0 + GLSL 130
        glsl_version = "#version 430";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT , GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT , GLFW_FALSE);
        
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primary);
           
        window = glfwCreateWindow(mode->width, mode->height, "Testing", NULL, NULL);
        if (window == NULL)
            return 1;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync


        int version = gladLoadGL(glfwGetProcAddress);
        if (version == 0) {
            printf("Failed to initialize OpenGL context\n");
            exit(0);
        }

        glfwSetCursorPosCallback(window, CursorPositionCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetScrollCallback(window, ScrollCallback);
        glfwSetKeyCallback(window, KeyCallback);

        // glDisable(GL_CULL_FACE);
        
        // glEnable(GL_STENCIL_TEST);    
        // glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  
        // glStencilFunc(GL_ALWAYS, 1, 0xFF); // all fragments should update the stencil buffer
        // glStencilMask(0xFF); // enable writing to the stencil buffer
        
        
        // glEnable(GL_BLEND);
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        glEnable(GL_DEPTH_TEST);
        // glEnable(GL_MULTISAMPLE);
        
        
        return 0;
    }    
    void InitImGUI() {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
    
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        ImGuiIO &_io = ImGui::GetIO();
        _io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    void Start() {
        demoManager={};
        ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        
        // Main loop
        while (!glfwWindowShouldClose(window))
        {
            GLenum err;
            while((err = glGetError()) != GL_NO_ERROR)
            {
                if(err==GL_INVALID_ENUM) std::cout << "GL_INVALID_ENUM"<<std::endl;
                if(err==GL_INVALID_VALUE) std::cout << "GL_INVALID_VALUE"<<std::endl;
                if(err==GL_INVALID_OPERATION) std::cout << "GL_INVALID_OPERATION"<<std::endl;
                if(err==GL_STACK_OVERFLOW) std::cout << "GL_STACK_OVERFLOW"<<std::endl;
                if(err==GL_STACK_UNDERFLOW) std::cout << "GL_STACK_UNDERFLOW"<<std::endl;
                if(err==GL_OUT_OF_MEMORY) std::cout << "GL_OUT_OF_MEMORY"<<std::endl;
                if(err==GL_INVALID_FRAMEBUFFER_OPERATION) std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION"<<std::endl;
                if(err==GL_CONTEXT_LOST) std::cout << "GL_CONTEXT_LOST"<<std::endl;
            }
            
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            RenderGUI();
            

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
            

            demoManager.windowWidth = display_w; 
            demoManager.windowHeight = display_h; 
            glDepthMask(GL_TRUE);
	
			demoManager.Render();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            

            glfwSwapBuffers(window);
        }

        demoManager.ClearDemo();

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void MouseMove(float x, float y) {
        demoManager.MouseMove(x, y);
    }

    void MouseAction(int button, int action, int mods) {
        demoManager.MouseAction(button, action, mods);
    }

    void Scroll(float yOffset) {
        demoManager.Scroll(yOffset);
    }
    void Key(int keyCode, int action)
    {
        demoManager.Key(keyCode, action);
    }
 

    GLFWwindow* window;
private:
    const char* glsl_version;
    DemoManager demoManager;


    void RenderGUI() {
        

        {
            ImGui::Begin("Demos : "); 
            if(ImGui::Button("Image Lab", ImVec2(100, 20))) {
                demoManager.ClearDemo();
                demoManager.LoadDemo(0);
            }
            if(ImGui::Button("Audio Lab", ImVec2(100, 20))) {
                demoManager.ClearDemo();
                demoManager.LoadDemo(1);
            }
            demoManager.RenderGUI();
            ImGui::End();
        }    
    }
};

App app;
void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    app.MouseMove((float)xpos, (float)ypos);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    app.MouseAction(button, action, mods);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    app.Scroll((float)yoffset);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    app.Key(key, action);
}


int main(int, char**)
{
    
    app.InitWindow();
    app.InitImGUI();
    app.Start();   
     
    return 0;
}