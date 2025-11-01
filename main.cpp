#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include "shader.h"

struct Ant {
    glm::vec2 pos;
    glm::vec2 vel;
    int carryingFood;
    float pad;
};

const int NUM_ANTS = 50000;
const int GRID_SIZE = 256;
const float MAX_SPEED = 10.0f;
const float DT = 0.016f;

GLuint createComputeShader(const char* path) {
    FILE* f = nullptr; //why does fopen not work in VS :(
    #ifdef _WIN32
        fopen_s(&f, path, "rb");   // Windows secure version
    #else
        f = fopen(path, "rb");     // Linux / macOS
    #endif

    if (!f) {
        std::cerr << "Failed to open " << path << "\n";
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    std::vector<char> src(len + 1);
    fread(src.data(), 1, len, f);
    fclose(f);
    src[len] = 0;

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* s = src.data();
    glShaderSource(shader, 1, &s, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Compute shader compile error:\n" << log << "\n";
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, shader);
    glLinkProgram(prog);
    glDeleteShader(shader);
    return prog;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(800, 800, "Ant uwu", 0, 0);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glPointSize(2.0f);
    std::vector<Ant> ants(NUM_ANTS);
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> d(-20.0f, 20.0f);
    for (auto& a : ants) {
        a.pos = glm::vec2(d(rng), d(rng));
        float angle = d(rng);
        a.vel = glm::normalize(glm::vec2(cos(angle), sin(angle))) * 0.1f;
        a.carryingFood = 0;
        a.pad = 0.0f;
    }
    GLuint antSSBO;
    glGenBuffers(1, &antSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, antSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ants.size() * sizeof(Ant), ants.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, antSSBO);

    GLuint pheromoneTex;
    glGenTextures(1, &pheromoneTex);
    glBindTexture(GL_TEXTURE_2D, pheromoneTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, GRID_SIZE, GRID_SIZE);
    std::vector<float> zeros(GRID_SIZE * GRID_SIZE, 0.0f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_SIZE, GRID_SIZE, GL_RED, GL_FLOAT, zeros.data());
    glBindImageTexture(1, pheromoneTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

    Shader renderShader("vertex.glsl", "fragment.glsl");
    GLuint computeProg = createComputeShader("compute.glsl");
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, antSSBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Ant), (void*)0);
    glEnableVertexAttribArray(0);

    glm::mat4 proj = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f);
    renderShader.use();
    renderShader.setMat4("projection", proj);

    GLuint groups = (NUM_ANTS + 256 - 1) / 256;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glUseProgram(computeProg);
        glUniform1f(glGetUniformLocation(computeProg, "dt"), DT);
        glUniform1f(glGetUniformLocation(computeProg, "maxSpeed"), MAX_SPEED);
        glUniform1i(glGetUniformLocation(computeProg, "gridSize"), GRID_SIZE);
        glDispatchCompute(groups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        renderShader.use();
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, NUM_ANTS);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
