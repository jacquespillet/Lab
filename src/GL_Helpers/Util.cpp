#include "Util.hpp"

#include <sstream>
#include <fstream>
#include <algorithm>


GL_Mesh *PlaneMesh(float sizeX, float sizeY, int subdivX, int subdivY)
{
    std::vector<unsigned int> triangles;
    std::vector<GL_Mesh::Vertex> vertices;   
    
    float xOffset = sizeX / (float)subdivX;
    float yOffset = sizeY / (float)subdivY;

    int numAdded=0;
    for(float y=-sizeX/2, yInx=0; yInx<subdivY; y+=yOffset, yInx++) {
        for(float x=-sizeY/2, xInx=0; xInx<subdivX; x+= xOffset, xInx++) {
            vertices.push_back({
                glm::vec3(x, 0, y),
                glm::vec3(0, 1, 0),
                glm::vec3(1, 0, 0),
                glm::vec3(0, 0, 1),
                glm::vec2(0,0)
            });

            if(xInx < subdivX-1 && yInx < subdivY-1) {
                triangles.push_back(numAdded);
                triangles.push_back(numAdded + subdivX + 1);
                triangles.push_back(numAdded + subdivX);
                
                triangles.push_back(numAdded);
                triangles.push_back(numAdded + 1);
                triangles.push_back(numAdded + subdivX + 1);
            }
            numAdded++;
        }
    }

    GL_Mesh *gl_mesh = new GL_Mesh(vertices, triangles);
    return gl_mesh;
}

void CreateComputeShader(std::string filename, GLint *programShaderObject)
{
    std::ifstream t(filename);
    std::stringstream shaderBuffer;
    shaderBuffer << t.rdbuf();
    std::string shaderSrc = shaderBuffer.str();
    GLint shaderObject;

    t.close();
	//make array to pointer for source code (needed for opengl )
	const char* vsrc[1];
	vsrc[0] = shaderSrc.c_str();
	
	//compile vertex and fragment shaders from source
	shaderObject = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shaderObject, 1, vsrc, NULL);
	glCompileShader(shaderObject);
	
	//link vertex and fragment shader to create shader program object
	*programShaderObject = glCreateProgram();
	glAttachShader(*programShaderObject, shaderObject);
	glLinkProgram(*programShaderObject);
	std::cout << "Shader:Compile: checking shader status" << std::endl;
	
	//Check status of shader and log any compile time errors
	int linkStatus;
	glGetProgramiv(*programShaderObject, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE) 
	{
		char log[5000];
		int lglen; 
		glGetProgramInfoLog(*programShaderObject, 5000, &lglen, log);
		std::cerr << "Shader:Compile: Could not link program: " << std::endl;
		std::cerr << log << std::endl;
		
        glGetShaderInfoLog(shaderObject, 5000, &lglen, log);
		std::cerr << "vertex shader log:\n" << log << std::endl;
		
        glDeleteProgram(*programShaderObject);
		*programShaderObject = 0;
	}
	else
	{
		std::cout << "Shader:Compile: compile success " << std::endl;
	}
}

void CreateComputeShader(const std::vector<std::string>& filenames, GLint *programShaderObject)
{
    std::stringstream shaderBuffer;
    shaderBuffer << "#version 430 core \n";
    for(int i=0; i<filenames.size(); i++)
    {
        std::ifstream t(filenames[i]);
        shaderBuffer << t.rdbuf();
        t.close();

        shaderBuffer << "\n";
    }
    std::string shaderSrc = shaderBuffer.str();
    GLint shaderObject;

	//make array to pointer for source code (needed for opengl )
	const char* vsrc[1];
	vsrc[0] = shaderSrc.c_str();
	
	//compile vertex and fragment shaders from source
	shaderObject = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shaderObject, 1, vsrc, NULL);
	glCompileShader(shaderObject);
	
	//link vertex and fragment shader to create shader program object
	*programShaderObject = glCreateProgram();
	glAttachShader(*programShaderObject, shaderObject);
	glLinkProgram(*programShaderObject);
	std::cout << "Shader:Compile: checking shader status" << std::endl;
	
	//Check status of shader and log any compile time errors
	int linkStatus;
	glGetProgramiv(*programShaderObject, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE) 
	{
		char log[5000];
		int lglen; 
		glGetProgramInfoLog(*programShaderObject, 5000, &lglen, log);
		std::cerr << "Shader:Compile: Could not link program: " << std::endl;
		std::cerr << log << std::endl;
		
        glGetShaderInfoLog(shaderObject, 5000, &lglen, log);
		std::cerr << "vertex shader log:\n" << log << std::endl;
		
        glDeleteProgram(*programShaderObject);
		*programShaderObject = 0;
	}
	else
	{
		std::cout << "Shader:Compile: compile success " << std::endl;
	}
}


void BindSSBO(GLuint shader, GLuint ssbo, std::string name, GLuint bindingPoint)
{
	glUseProgram(shader);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, ssbo);
}

float Clamp01(float input)
{
	float Result = std::min(1.0f, std::max(0.001f, input));
	return Result;
}

float Clamp(float input, float min, float max)
{
	float Result = std::min(max, std::max(min, input));
	return Result;
}


float Lerp(float a, float b, float f)
{
    return a + f * (b - a);
}  

float RandomFloat(float min, float max)
{
	float Result = (float)std::rand() / (float)RAND_MAX;
	float range = max - min;
	Result *= range;
	Result += min;
	return Result;
}

int RandomInt(int min, int max)
{
	float ResultF = RandomFloat((float)min, (float)max);
	int Result = (int)ResultF;
	return Result;
}

glm::vec3 RandomVec3(glm::vec3 min, glm::vec3 max)
{
    glm::vec3 Result = {
        RandomFloat(min.x, max.x),
        RandomFloat(min.y, max.y),
        RandomFloat(min.z, max.z)
    };
    return Result;
}

bool RayTriangleIntersection( 
    glm::vec3 &orig, glm::vec3 &dir, 
    glm::vec3 &v0,glm::vec3 &v1,glm::vec3 &v2,
    float *t, glm::vec2*uv) 
{ 
    float kEpsilon = 1e-8f; 
    float u, v;

    glm::vec3 v0v1 = v1 - v0; 
    glm::vec3 v0v2 = v2 - v0; 
    glm::vec3 pvec = glm::cross(dir, v0v2);
    float det = glm::dot(v0v1, pvec);

    // ray and triangle are parallel if det is close to 0
    if (std::abs(det) < kEpsilon) return false; 

    float invDet = 1 / det; 
   
    glm::vec3 tvec = orig - v0; 
    u = glm::dot(tvec, pvec) * invDet; 
    if (u < 0 || u > 1) return false; 

    glm::vec3 qvec = glm::cross(tvec, v0v1); 
    v = glm::dot(dir, qvec) * invDet; 
    if (v < 0 || u + v > 1) return false; 

    *t = glm::dot(v0v2, qvec) * invDet; 
    *uv = glm::vec2(u, v);

    // return true;
	if (*t > 0) return true;
	else return false; 
} 

bool RayAABBIntersection(glm::vec3 &orig, glm::vec3 &invDir, int *sign, AABB &aabb)
{
    float tmin, tmax, tymin, tymax, tzmin, tzmax; 
 
    tmin = (aabb.bounds[sign[0]].x - orig.x) * invDir.x; 
    tmax = (aabb.bounds[1-sign[0]].x - orig.x) * invDir.x; 
    tymin = (aabb.bounds[sign[1]].y - orig.y) * invDir.y; 
    tymax = (aabb.bounds[1-sign[1]].y - orig.y) * invDir.y; 
 
    if ((tmin > tymax) || (tymin > tmax)) 
        return false; 
    if (tymin > tmin) 
        tmin = tymin; 
    if (tymax < tmax) 
        tmax = tymax; 
 
    tzmin = (aabb.bounds[sign[2]].z - orig.z) * invDir.z; 
    tzmax = (aabb.bounds[1-sign[2]].z - orig.z) * invDir.z; 
 
    if ((tmin > tzmax) || (tzmin > tmax)) 
        return false; 
    if (tzmin > tmin) 
        tmin = tzmin; 
    if (tzmax < tmax) 
        tmax = tzmax; 
 
    return true;
}


void CalculateTangents(std::vector<GL_Mesh::Vertex>& vertices, std::vector<uint32_t> &triangles) {
	std::vector<glm::vec4> tan1(vertices.size(), glm::vec4(0));
	std::vector<glm::vec4> tan2(vertices.size(), glm::vec4(0));
	
    for(uint64_t i=0; i<triangles.size(); i+=3) {
		glm::vec3 v1 = vertices[triangles[i]].position;
		glm::vec3 v2 = vertices[triangles[i + 1]].position;
		glm::vec3 v3 = vertices[triangles[i + 2]].position;

		glm::vec2 w1 = vertices[triangles[i]].uv;
		glm::vec2 w2 = vertices[triangles[i+1]].uv;
		glm::vec2 w3 = vertices[triangles[i+2]].uv;

		double x1 = v2.x - v1.x;
		double x2 = v3.x - v1.x;
		double y1 = v2.y - v1.y;
		double y2 = v3.y - v1.y;
		double z1 = v2.z - v1.z;
		double z2 = v3.z - v1.z;

		double s1 = w2.x - w1.x;
		double s2 = w3.x - w1.x;
		double t1 = w2.y - w1.y;
		double t2 = w3.y - w1.y;

  		double r = 1.0F / (s1 * t2 - s2 * t1);
		glm::vec4 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r, 0);
		glm::vec4 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r, 0);

		tan1[triangles[i]] += sdir;
		tan1[triangles[i + 1]] += sdir;
		tan1[triangles[i + 2]] += sdir;
		
		tan2[triangles[i]] += tdir;
		tan2[triangles[i + 1]] += tdir;
		tan2[triangles[i + 2]] += tdir;
	}

	for(uint64_t i=0; i<vertices.size(); i++) { 
		glm::vec3 n = vertices[i].normal;
		glm::vec3 t = glm::vec3(tan1[i]);

		vertices[i].tan = glm::normalize((t - n * glm::dot(n, t)));
    
        float w = (glm::dot(glm::cross(n, t), glm::vec3(tan2[i])) < 0.0F) ? -1.0F : 1.0F;
		vertices[i].bitan =  glm::normalize(glm::cross(n, t) * w);
	}
}


GL_Mesh *GetQuad()
{
    std::vector<GL_Mesh::Vertex> vertices = 
    {
        {
            glm::vec3(-1.0, -1.0, 0), 
            glm::vec3(0), 
            glm::vec3(0), 
            glm::vec3(0),
            glm::vec2(0, 0)
        },
        {
            glm::vec3(-1.0, 1.0, 0), 
            glm::vec3(0), 
            glm::vec3(0), 
            glm::vec3(0),
            glm::vec2(0, 1)
        },
        {
            glm::vec3(1.0, 1.0, 0), 
            glm::vec3(0), 
            glm::vec3(0), 
            glm::vec3(0),
            glm::vec2(1, 1)
        },
        {
            glm::vec3(1.0, -1.0, 0), 
            glm::vec3(0), 
            glm::vec3(0), 
            glm::vec3(0),
            glm::vec2(1, 0)
        }
    };

    std::vector<uint32_t> triangles = {0,2,1,3,2,0};
    GL_Mesh *quad = new GL_Mesh(vertices, triangles);	
    return quad;
}