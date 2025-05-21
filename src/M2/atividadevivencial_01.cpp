/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 13/08/2024
 * 
 * Modificado pelos alunos Anderson Koefender, Lucas Luan Rost e Raphael Ferracioli
 * para a Atividade Vivencial 01.
 *  
 * Modificado para Atividade Vivencial 01 pelos alunos
 * 	Anderson Koefender
 * 	Lucas Luan Rost
 * 	Raphael Ferracioli
 */

 #include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

// Dimensoes da janela (pode ser alterado em tempo de execucao)
const GLuint WIDTH = 800, HEIGHT = 600;

// Vetor para armazenar os pontos clicados (coordenadas normalizadas OpenGL)
vector<GLfloat> clickedPoints;

// Estrutura para armazenar dados de um triangulo, incluindo sua cor
struct Triangle {
    GLuint VAO;
    GLuint VBO;
    GLfloat r, g, b;
};

vector<Triangle> triangles;

// Esta funcao cria um triangulo com base em tres pontos e cor aleatoria
GLuint createTriangle(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat& r, GLfloat& g, GLfloat& b) {
    GLfloat vertices[] = {
        x0, y0, 0.0f,
        x1, y1, 0.0f,
        x2, y2, 0.0f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Gera cor aleatoria para o triangulo
    r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

    Triangle t = { VAO, VBO, r, g, b };
    triangles.push_back(t);

    return VAO;
}

// Funcao de callback chamada sempre que um botao do mouse for pressionado
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float x = (xpos / WIDTH) * 2.0f - 1.0f;
        float y = -((ypos / HEIGHT) * 2.0f - 1.0f);

        clickedPoints.push_back(x);
        clickedPoints.push_back(y);

        cout << "Vertice armazenado: (" << x << ", " << y << ")\n";

        if (clickedPoints.size() == 6) {
            cout << "Triangulo formado pelos vertices:\n";
            for (int i = 0; i < 3; i++) {
                cout << "(" << clickedPoints[i * 2] << ", " << clickedPoints[i * 2 + 1] << ")\n";
            }
            GLfloat r, g, b;
            createTriangle(
                clickedPoints[0], clickedPoints[1],
                clickedPoints[2], clickedPoints[3],
                clickedPoints[4], clickedPoints[5],
                r, g, b
            );
            clickedPoints.clear();
        }
    }
}

// Codigo fonte do Vertex Shader (em GLSL): ainda hardcoded
const char* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
void main()
{
    gl_Position = vec4(position, 1.0);
})";

// Codigo fonte do Fragment Shader (em GLSL)
const char* fragmentShaderSource = R"(
#version 400
uniform vec3 inputColor;
out vec4 color;
void main()
{
    color = vec4(inputColor, 1.0);
})";

// Esta funcao esta basntante hardcoded - objetivo eh compilar e "buildar" um programa de
//  shader simples e unico neste exemplo de codigo
//  O codigo fonte do vertex e fragment shader esta nos arrays vertexShaderSource e
//  fragmentShader source no inicio deste arquivo
//  A funcao retorna o identificador do programa de shader
int setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    glfwInit();

    // Criacao da janela GLFW
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade Vivencial 01", NULL, NULL);
    glfwMakeContextCurrent(window);

    // GLAD: carrega todos os ponteiros de funcoes da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cerr << "Erro ao inicializar GLAD." << endl;
        return -1;
    }

    // Fazendo o registro da funcao de callback para a janela GLFW
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Definindo as dimensoes da viewport com as mesmas dimensoes da janela da aplicacao
    glViewport(0, 0, WIDTH, HEIGHT);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");

    // Loop da aplicacao - "game loop"
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Limpa o buffer de cor
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (const auto& tri : triangles) {
            glUniform3f(colorLoc, tri.r, tri.g, tri.b);
            glBindVertexArray(tri.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }

    // Pede pra OpenGL desalocar os buffers
    for (const auto& tri : triangles) {
        glDeleteVertexArrays(1, &tri.VAO);
        glDeleteBuffers(1, &tri.VBO);
    }

    // Finaliza a execucao da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}
