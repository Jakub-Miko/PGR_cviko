#include <iostream>
#include <SDL3/SDL.h>
#include <geGL/geGL.h>
#include <geGL/StaticCalls.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace ge::gl;

uint CompileShader(const std::string& source, uint type) {
    std::ifstream file(source);
    if (!file.is_open()) {
        throw std::runtime_error("File " + source + "could not be opened.");
    }
    std::stringstream ss;
    ss << file.rdbuf();
    auto src = ss.str();

    uint shader = glCreateShader(type);
    GLint size = src.size();
    auto str_src = src.data();
    glShaderSource(shader, 1, &str_src, &size);
    glCompileShader(shader);
    auto error = glGetError();
    if(error != 0) {
        throw std::runtime_error("Shader " + source + " could not be compiled with error: " + std::to_string(error));
    }
    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if(!result) {
        char message[500];
        glGetShaderInfoLog(shader, 500, nullptr, message);
        throw std::runtime_error(message);
    }

    file.close();
    return shader;
}


uint LinkProgram(uint vs, uint fs) { 
    uint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if(!result) {
        char message[500]; 
        glGetProgramInfoLog(program, 500, nullptr, message);
        throw std::runtime_error(message);
    }
    return program;
}

struct Vertex {
    glm::vec4 position;
    glm::vec3 normal;
};

int main() {
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); 
    auto window = SDL_CreateWindow("WindowPGR", 1024, 768, SDL_WINDOW_OPENGL);
    auto ctx = SDL_GL_CreateContext(window);
    bool running = true;
    ge::gl::init();
    glEnable(GL_DEPTH_TEST);

    Assimp::Importer importer;
    std::string scene_path = "../bunny.obj";
    auto scene = importer.ReadFile(scene_path, aiProcess_Triangulate | aiProcess_GenNormals);
    
    if(!scene || !scene->HasMeshes()) {
        throw std::runtime_error("Scene " + scene_path + " could not be loaded.");
    }

    auto mesh = scene->mMeshes[0];

    auto vb_size = mesh->mNumVertices;
    auto index_buf_size = mesh->mNumFaces * 3;
    auto index_buffer_source = new uint[index_buf_size];
    auto vertex_buffer_source = new Vertex[vb_size];
    for(int i = 0; i < mesh->mNumFaces; i++) {
        auto& face = mesh->mFaces[i];
        if(face.mNumIndices != 3) {
            throw std::runtime_error("Invalid mesh.");
        }
        index_buffer_source[i * 3] = face.mIndices[0];
        index_buffer_source[i * 3 + 1] = face.mIndices[1];
        index_buffer_source[i * 3 + 2] = face.mIndices[2];
    }

     for(int i = 0; i < mesh->mNumVertices; i++) {
        auto& vertex = mesh->mVertices[i];
        auto& normal = mesh->mNormals[i];
        vertex_buffer_source[i] = Vertex{glm::vec4(vertex.x, vertex.y, vertex.z, 1.0f), glm::vec3(normal.x, normal.y, normal.z) };
    }

    uint index_buffer, vertex_buffer;
    glCreateBuffers(1, &index_buffer);
    glCreateBuffers(1, &vertex_buffer);

    glNamedBufferData(index_buffer, index_buf_size * sizeof(uint), index_buffer_source, GL_STATIC_DRAW);
    glNamedBufferData(vertex_buffer, vb_size * sizeof(Vertex), vertex_buffer_source, GL_STATIC_DRAW);

    uint vao;
    glCreateVertexArrays(1, &vao);
    glVertexArrayVertexBuffer(vao, 0, vertex_buffer, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(vao, index_buffer);
    glEnableVertexArrayAttrib(vao, 0);
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(vao, 1, 0);

    glBindVertexArray(vao);

    glm::mat4 model_matrix(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f/9, 0.01f, 100.0f);
    glm::mat4 view_matrix = glm::lookAt(glm::vec3(0.2,0.2,0.5f), glm::vec3(0.0f,0.1f, 0.0f), glm::vec3(0,1,0));

    delete[] index_buffer_source;
    delete[] vertex_buffer_source;
    
    uint vs = CompileShader("../vertex_shader.glsl", GL_VERTEX_SHADER);
    uint fs = CompileShader("../fragment_shader.glsl", GL_FRAGMENT_SHADER);
    uint program = LinkProgram(vs, fs);

    auto mvp = projection * view_matrix * model_matrix;
    auto normal_mat = glm::transpose(glm::inverse(view_matrix * model_matrix));
    glm::vec3 sun_dir(0,1,0);
    glUseProgram(program);

    uint mvp_name = glGetUniformLocation(program,"mvp");  
    uint normal_mat_name = glGetUniformLocation(program,"normal_mat");  
    uint sun_dir_name = glGetUniformLocation(program,"sun_dir");  
    
    glUniformMatrix4fv(mvp_name, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(normal_mat_name, 1, GL_FALSE, glm::value_ptr(normal_mat));
    glUniform3fv(sun_dir_name, 1, glm::value_ptr(sun_dir));
    while (running)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if( event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glDrawElements(GL_TRIANGLES, index_buf_size, GL_UNSIGNED_INT, nullptr);

        SDL_GL_SwapWindow(window);
    }

    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &index_buffer);
    glDeleteVertexArrays(1, &vao);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(program);
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
}