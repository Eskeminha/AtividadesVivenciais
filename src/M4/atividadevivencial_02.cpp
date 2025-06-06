/*
 * MultiSprite.cpp
 *
 * Exemplo de exercício de Processamento Gráfico: vários retângulos (sprites) texturizados.
 * Baseado no HelloSprite.cpp fornecido em aula.
 *
 * - Cria uma classe Sprite que encapsula VAO, textura, posição, escala e rotação.
 * - Usa projeção ortográfica para mapear (0,0)-(800,600) em coordenadas de tela (px).
 * - Desenha múltiplos sprites com texturas diferentes em posições/escala/rotação variadas.
 *
 * Para compilar (exemplo com g++ no Linux):
 *   g++ MultiSprite.cpp -o MultiSprite \
 *       -lglfw -ldl -lGL \
 *       -I/path/para/glad/include \
 *       -I/path/para/glm \
 *       -I/path/para/stb_image \
 * 
 * Atenção: este código foi adaptado para incluir:
 *   - Movimento para a direita/esquerda com limite de tela.
 *   - Flip horizontal ao andar para a esquerda.
 *   - Pulo com física simples (velocidade inicial + gravidade).
 *   - Efeito parallax de 5 camadas usando texturas extras ("parallax1.png" até "parallax5.png").
 *   - Mantidos todos os comentários originais e as texturas já carregadas, exceto o SKY, que foi removido.
 */

#include <iostream>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const GLuint SCREEN_WIDTH  = 800;
const GLuint SCREEN_HEIGHT = 600;

// =========================================================================
// 1) Shaders (vertex/fragment) para desenhar um quad texturizado
// =========================================================================

// GLSL: vertex shader
const GLchar* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 uModel;
uniform mat4 uProjection;
out vec2 TexCoord;
void main()
{
    gl_Position = uProjection * uModel * vec4(aPos, 1.0);
    TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
}
)";

// GLSL: fragment shader
const GLchar* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
void main()
{
    FragColor = texture(uTexture, TexCoord);
}
)";

// =========================================================================
// 2) Funções utilitárias para criar e compilar shaders, criar quad VAO, carregar texturas
// =========================================================================

// Compila e retorna ID do programa de shader
GLuint CreateShaderProgram(const GLchar* vShaderSrc, const GLchar* fShaderSrc)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderSrc, nullptr);
    glCompileShader(vertexShader);
    GLint success; GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "[ERRO] Vertex Shader compilation failed:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderSrc, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "[ERRO] Fragment Shader compilation failed:\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "[ERRO] Shader Program link failed:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// Carrega textura de arquivo e retorna ID da textura
GLuint loadTexture(const std::string& filePath)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Parâmetros de wrapping e filtragem
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 1 ? GL_RED
                         : (nrChannels == 3 ? GL_RGB
                                            : GL_RGBA));
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cerr << "[ERRO] Falha ao carregar textura em: " << filePath << std::endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

// Cria VAO + VBO para um quad (2 triângulos) centrado na origem, usando TRIANGLE_STRIP
GLuint CreateQuadVAO()
{
    GLfloat quadVertices[] = {
        // Posições        // Coords de textura
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, // topo-esquerda
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // baixo-esquerda
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f, // topo-direita
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f  // baixo-direita
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // 1. Configura atributo “pos” (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // 2. Configura atributo “texCoord” (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // 3. Limpa bindings
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

// =========================================================================
// 3) Callback para teclado (ESC fecha a janela)
// =========================================================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// =========================================================================
// 4) Classe Sprite
// =========================================================================

class Sprite {
public:
    // Construtor: recebe ID de shader compilado, ID de VAO (quad base), e textura
    Sprite(GLuint shaderID, GLuint quadVAO, GLuint textureID)
        : shaderID(shaderID),
          VAO(quadVAO),
          textureID(textureID),
          facingRight(true),
          isJumping(false),
          velocityY(0.0f),
          gravity(800.0f),
          initialJumpV(350.0f),
          rotation(0.0f)
    {
        position = glm::vec2(0.0f, 0.0f);
        scale    = glm::vec2(100.0f, 100.0f); // escala padrão: 100 × 100 px
        groundY  = position.y;               // inicializa "chão" para pulo
    }

    // Ajusta posição (em px, no sistema de coordenadas [0,800]×[0,600])
    void SetPosition(float x, float y) {
        position = glm::vec2(x, y);
        groundY  = y; // define o chão para o pulo
    }

    // Ajusta escala (largura×altura em px)
    void SetScale(float widthPx, float heightPx) {
        scale = glm::vec2(widthPx, heightPx);
    }

    // Ajusta rotação em graus
    void SetRotation(float angleDegrees) {
        rotation = angleDegrees;
    }

    // Movimentos Direita, esquerda, cima e baixo
    void andaDireita(float qtd) {
        // Limita para que o sprite não ultrapasse a borda direita da tela
        if (position.x + qtd + scale.x / 2.0f < SCREEN_WIDTH) {
            position.x += qtd;
        }
        facingRight = true; // flip para a direita
    }

    void andaEsquerda(float qtd) {
        // Limita para que o sprite não ultrapasse a borda esquerda da tela
        if (position.x + qtd - scale.x / 2.0f > 0) {
            position.x += qtd;
        }
        facingRight = false; // flip para a esquerda
    }

    void andaCima(float /*qtd*/) {
        // Adicionar lógica de pulo: sobe até certa altura e depois cai
        if (!isJumping) {
            isJumping = true;
            velocityY = initialJumpV; // define velocidade inicial do pulo (px/s)
        }
    }

    void andaBaixo(float qtd) {
        // Caso queira mover para baixo (não usado para pulo)
        position.y += qtd;
    }

    // Desenha o sprite (quad texturizado) usando shader e textura definidos
    void Draw(const glm::mat4& projection) {
        glm::mat4 model = glm::mat4(1.0f);

        // 1. Translada para a posição desejada (posição refere-se ao CENTRO do sprite)
        model = glm::translate(model, glm::vec3(position, 0.0f));
        // 2. Move o pivô para o centro do quad (antes da rotação)
        model = glm::translate(model, glm::vec3(0.5f * scale.x, 0.5f * scale.y, 0.0f));
        // 3. Rotaciona (se houver rotação)
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        // 4. Retorna o pivô para o canto inferior-esquerdo
        model = glm::translate(model, glm::vec3(-0.5f * scale.x, -0.5f * scale.y, 0.0f));

        // 5. Aplica flip horizontal se estiver virado para a esquerda
        float flipX = (facingRight ? +1.0f : -1.0f);
        model = glm::scale(model, glm::vec3(flipX * scale.x, scale.y, 1.0f));

        // 6. Envia uModel e uProjection para o shader
        glUseProgram(shaderID);
        GLint modelLoc = glGetUniformLocation(shaderID, "uModel");
        GLint projLoc  = glGetUniformLocation(shaderID, "uProjection");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

        // 7. Textura na unidade 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderID, "uTexture"), 0);

        // 8. Bind no VAO (quad) e desenha
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    // Atualiza a física do pulo (gravidade, subida e descida)
    void UpdatePhysics(float deltaTime) {
        if (isJumping) {
            // Move para cima/baixo de acordo com velocidadeY atual
            position.y += velocityY * deltaTime;
            // Aplica gravidade (reduz velocidadeY)
            velocityY -= gravity * deltaTime;
            // Se voltar ao chão, para o pulo
            if (position.y <= groundY) {
                position.y   = groundY;
                isJumping    = false;
                velocityY    = 0.0f;
            }
        }
    }

    // Expondo posição e escala publicamente para facilitar uso no parallax
    glm::vec2 position;
    glm::vec2 scale;

private:
    GLuint shaderID;
    GLuint VAO;
    GLuint textureID;

    float rotation;    // rotação em graus
    bool  facingRight; // controla flip horizontal

    // Variáveis para pulo
    bool  isJumping;
    float velocityY;
    float gravity;      // aceleração da gravidade (px/s²)
    float initialJumpV; // velocidade inicial do pulo (px/s)
    float groundY;      // posição Y do chão (onde o sprite começa)
};

// Função auxiliar (não utilizada diretamente, mas deixada para referência)
// recebe uma instância de Sprite, a janela e um valor qtd para movimentar conforme teclas
void move(Sprite& sprite, GLFWwindow* window, float qtd) {
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        sprite.andaDireita(qtd);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        sprite.andaEsquerda(qtd);
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        sprite.andaCima(qtd);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        sprite.andaBaixo(qtd);
    }
}

int main()
{
    // 6.1 Inicializa GLFW
    if (!glfwInit()) {
        std::cerr << "[ERRO] Falha ao inicializar GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // anti-aliasing
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
                                          "MultiSprite Example", nullptr, nullptr);
    if (!window) {
        std::cerr << "[ERRO] Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // 6.2 Carrega GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[ERRO] Falha ao inicializar GLAD" << std::endl;
        return -1;
    }
    // 6.3 Ajusta viewport
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // 6.4 Compila e linka shader
    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    // 6.5 Cria VAO base (quad 1×1)
    GLuint quadVAO = CreateQuadVAO();

    // 6.6 Carrega texturas (exemplo: várias texturas já existentes)
    GLuint clouds   = loadTexture("../assets/sprites/clouds_1.png");
    GLuint clouds2  = loadTexture("../assets/sprites/clouds_2.png");
    GLuint clouds3  = loadTexture("../assets/sprites/clouds_3.png");
    GLuint clouds4  = loadTexture("../assets/sprites/clouds_4.png");
    GLuint rocks    = loadTexture("../assets/sprites/rocks_1.png");
    GLuint rocks2   = loadTexture("../assets/sprites/rocks_2.png");
    GLuint jare     = loadTexture("../assets/sprites/jare.png");
    GLuint fafare   = loadTexture("../assets/sprites/fafare.png");
    GLuint raphare  = loadTexture("../assets/sprites/raphare.png");

    // 6.6.1 Carrega texturas de parallax (camadas extras; substitua pelos arquivos reais)
    GLuint parallaxTex[5];
    parallaxTex[0] = loadTexture("../assets/sprites/parallax1.png"); // camada mais distante
    parallaxTex[1] = loadTexture("../assets/sprites/parallax2.png");
    parallaxTex[2] = loadTexture("../assets/sprites/parallax3.png");
    parallaxTex[3] = loadTexture("../assets/sprites/parallax4.png");
    parallaxTex[4] = loadTexture("../assets/sprites/parallax5.png"); // camada mais próxima

    // 6.7 Cria instâncias de Sprite
    std::vector<Sprite> sprites;

    // JARE ON TOP (jogador/personagem)
    sprites.emplace_back(shaderProgram, quadVAO, jare);
    // Posição inicial do jogador: em baixo, próximo ao "chão" y = 50
    sprites.back().SetPosition(50.0f, 50.0f);
    sprites.back().SetScale(128.0f, 128.0f);
    sprites.back().SetRotation(0.0f);

    // (adicione mais sprites conforme desejar)
    // Por exemplo, se quiser usar 'fafare' ou 'raphare', pode emplacá-los aqui:
    // sprites.emplace_back(shaderProgram, quadVAO, fafare);
    // sprites.back().SetPosition(200.0f, 100.0f);
    // sprites.back().SetScale(64.0f, 64.0f);
    // sprites.back().SetRotation(0.0f);

    // 6.7.1 Cria sprites de parallax (cada camada ocupa toda a tela; posição inicial em x=0)
    std::vector<Sprite> parallaxSprites;
    parallaxSprites.reserve(5);
    for (int i = 0; i < 5; ++i) {
        parallaxSprites.emplace_back(shaderProgram, quadVAO, parallaxTex[i]);
        // Define escala para toda a tela, mas posição X será ajustada via offset
        parallaxSprites.back().SetScale((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
        // Verticalmente, centraliza no meio da tela (300)
        parallaxSprites.back().SetPosition(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
        parallaxSprites.back().SetRotation(0.0f);
    }

    float MULTIPLICADOR = 10.0f;

    // 6.7.2 Define velocidades de parallax (quanto maior, mais rápido a camada se move)
    float parallaxSpeeds[5] = { 20.0f * MULTIPLICADOR, 40.0f * MULTIPLICADOR, 60.0f * MULTIPLICADOR, 80.0f * MULTIPLICADOR, 100.0f * MULTIPLICADOR };

    // 6.7.3 Offset X inicial de cada camada (em px, partindo de 0 = sem deslocamento)
    float parallaxOffsetX[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // 6.8 Define matriz de projeção ortográfica (coordenadas de tela)
    glm::mat4 projection = glm::ortho(
        0.0f, (float)SCREEN_WIDTH,
        0.0f, (float)SCREEN_HEIGHT,
       -1.0f,  1.0f
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 6.9 Variáveis de tempo para deltaTime
    float lastFrameTime = (float)glfwGetTime();

    // 6.10 Game Loop
    while (!glfwWindowShouldClose(window)) {
        // 6.10.1 Calcula deltaTime (em segundos)
        float currentFrame = (float)glfwGetTime();
        float deltaTime    = currentFrame - lastFrameTime;
        lastFrameTime      = currentFrame;

        // 6.10.2 Processa eventos
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // 6.10.3 Entrada de teclado PARA MOVIMENTO (horizontal e pulo) de forma independente
        int moveDir = 0; // -1 = andando para a esquerda, +1 = para a direita, 0 = parado

        // *** Movimentar o JOGADOR que está em sprites[0] ***
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            sprites[0].andaDireita(200.0f * MULTIPLICADOR * deltaTime); // 200 px/s
            moveDir = +1;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            sprites[0].andaEsquerda(-200.0f * MULTIPLICADOR * deltaTime); // qtd negativo = moveDir -1
            moveDir = -1;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            sprites[0].andaCima(0.0f);
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            sprites[0].andaBaixo(200.0f * deltaTime);
        }

        // 6.10.4 Atualiza física do pulo do jogador (sprites[0])
        sprites[0].UpdatePhysics(deltaTime);

        // 6.10.5 Atualiza offsets de parallax com base na direção do jogador
        for (int i = 0; i < 5; ++i) {
            parallaxOffsetX[i] -= moveDir * parallaxSpeeds[i] * deltaTime;

            // Faz wrapping do offset quando sai muito para a esquerda ou direita
            if (parallaxOffsetX[i] <= -SCREEN_WIDTH)
                parallaxOffsetX[i] += SCREEN_WIDTH;
            if (parallaxOffsetX[i] >= SCREEN_WIDTH)
                parallaxOffsetX[i] -= SCREEN_WIDTH;
        }

        // 6.10.6 Desenha cena
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 6.10.6.1 Desenha cada camada de parallax (da mais distante à mais próxima)
        for (int i = 0; i < 5; ++i) {
            // Calcula o centro X atual da textura: offset + metade da largura da tela
            float centerX = parallaxOffsetX[i] + (SCREEN_WIDTH / 2.0f);
            float centerY = SCREEN_HEIGHT / 2.0f; // mantém vertical centralizada

            // Se o offset estiver ≤ 0: desenha em centerX e centerX + SCREEN_WIDTH
            // Senão, desenha em centerX e centerX - SCREEN_WIDTH
            if (parallaxOffsetX[i] <= 0.0f) {
                // 1ª cópia
                parallaxSprites[i].SetPosition(centerX, centerY);
                parallaxSprites[i].Draw(projection);
                // 2ª cópia (à direita)
                parallaxSprites[i].SetPosition(centerX + SCREEN_WIDTH, centerY);
                parallaxSprites[i].Draw(projection);
            } else {
                // 1ª cópia
                parallaxSprites[i].SetPosition(centerX, centerY);
                parallaxSprites[i].Draw(projection);
                // 2ª cópia (à esquerda)
                parallaxSprites[i].SetPosition(centerX - SCREEN_WIDTH, centerY);
                parallaxSprites[i].Draw(projection);
            }
        }

        // 6.10.6.2 Desenha os sprites originais (jogador, rochas, etc.) por cima dos backgrounds
        for (auto& sprite : sprites) {
            sprite.Draw(projection);
        }

        // 6.10.7 Troca buffers
        glfwSwapBuffers(window);
    }

    // 6.11 Finaliza
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
