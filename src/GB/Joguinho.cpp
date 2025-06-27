/*
 * Hello Triangle - Código adaptado de:
 *   - https://learnopengl.com/#!Getting-started/Hello-Triangle
 *   - https://antongerdelan.net/opengl/glcontext2.html
 *
 * Adaptado por: Rossana Baptista Queiroz
 *
 * Disciplinas:
 *   - Processamento Gráfico (Ciência da Computação - Híbrido)
 *   - Processamento Gráfico: Fundamentos (Ciência da Computação - Presencial)
 *   - Fundamentos de Computação Gráfica (Jogos Digitais)
 *
 * Descrição:
 *   Este código é o "Olá Mundo" da Computação Gráfica, utilizando OpenGL Moderna.
 *   No pipeline programável, o desenvolvedor pode implementar as etapas de
 *   Processamento de Geometria e Processamento de Pixel utilizando shaders.
 *   Um programa de shader precisa ter, obrigatoriamente, um Vertex Shader e um Fragment Shader,
 *   enquanto outros shaders, como o de geometria, são opcionais.
 *
 * Histórico:
 *   - Versão inicial: 07/04/2017
 *   - Última atualização: 18/03/2025
 *
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//GLM
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

 #define TILEMAP_WIDTH 15
 #define TILEMAP_HEIGHT 15

struct Sprite
{
	GLuint VAO;
	GLuint texID;
	vec3 position;
	vec3 dimensions; //tamanho do frame
	float ds, dt;
	int iAnimation, iFrame;
	int nAnimations, nFrames;
};
	
struct Tile
{
	GLuint VAO;
	GLuint texID; // de qual tileset
	int iTile; //indice dele no tileset
	vec3 position;
	vec3 dimensions; //tamanho do losango 2:1
	float ds, dt;
	bool caminhavel;
	//int iAnimation, int iFrame;
	//int nAnimations, nFrames;
};	

// Estrutura para personagem animado
struct Personagem {
	GLuint VAO;
	GLuint texID;
	vec3 position;
	vec3 dimensions;
	float ds, dt;
	int direcao; // 0=N, 1=NE, 2=L, 3=SE, 4=S, 5=SO, 6=O, 7=NO
	int frame;
	int nDirecoes;
	int nFrames;
	bool andando = false; // novo campo para controle de movimento
	int grupoAnimacao = 0; // 0: frames baixos, 1: frames altos
};

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupSprite(int nAnimations, int nFrames, float &ds, float &dt);
int setupTile(int nTiles, float &ds, float &dt);
int loadTexture(string filePath, int &width, int &height);
void desenharMapa(GLuint shaderID);
void desenharPersonagem(GLuint shaderID);
void leMapa(const std::string& path, int map[][TILEMAP_WIDTH]);
void imprimeMapa(int map[][TILEMAP_WIDTH]);
void desenharMoedas(GLuint shaderID);
void verificaEventoMapa(int posx, int posy);
void liberarTileComAnimacao(int x, int y, bool &evento_ativado);
void desenharGreatJareSpirit(GLuint shaderID);
void coletarMoeda(int indice);
void drawText_GL33(float x, float y, const char* text, float r, float g, float b, float scale);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 960, HEIGHT = 720;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
 #version 400
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec2 texc;
 out vec2 tex_coord;
 uniform mat4 model;
 uniform mat4 projection;
 void main()
 {
	tex_coord = vec2(texc.s, 1.0 - texc.t);
	gl_Position = projection * model * vec4(position, 1.0);
 }
 )";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
 #version 400
 in vec2 tex_coord;
 out vec4 color;
 uniform sampler2D tex_buff;
 uniform vec2 offsetTex;

 void main()
 {
	 color = texture(tex_buff,tex_coord + offsetTex);
 }
 )";



int map[TILEMAP_WIDTH][TILEMAP_HEIGHT]; // Mapa principal
int barreiras[TILEMAP_WIDTH][TILEMAP_HEIGHT]; // Mapa das barreiras
Personagem migore; // Personagem principal
Sprite moedas;  // Sprite das moedas
Sprite greatJareSpirit; // Sprite do great_jare_spirit
Sprite topBar; // Sprite da barra superior
bool greatJareSpirit_ativo = false; // Controla se o sprite será exibido
double pontuacao = 0;
int vidas = 5;

// INICIALIZA OS CONTROLES DE EVENTOS
static bool evento_38_ativado = false;
static bool evento_210_ativado = false;
static bool evento_45_ativado = false;
static bool evento_512_ativado = false;
static bool evento_82_ativado = false;
static bool evento_83_ativado = false;
static bool evento_71_ativado = false;
static bool evento_41_ativado = false;
static bool evento_118_ativado = false;
static bool evento_1312_ativado = false;
static bool evento_jare_capturado = false;

// --- Controle de animação de troca de tile ---
struct AnimacaoTile {
    bool ativa = true;
    int x = 0, y = 0;
    int tileInicial = 0;
    int tileFinal = 0;
    double tempo = 0.0;
    double tempoTotal = 1.0; // duração total da animação em segundos
};
static std::vector<AnimacaoTile> animacoesTile;

void animarTrocaTile(int x, int y, int tileFinal) {
    AnimacaoTile novaAnim;
    novaAnim.x = x;
    novaAnim.y = y;
    novaAnim.tileInicial = map[x][y];
    novaAnim.tileFinal = tileFinal;
    novaAnim.tempo = 0.0;
    novaAnim.tempoTotal = 1.0;
    novaAnim.ativa = true;
    animacoesTile.push_back(novaAnim);
}

// Struct para representar uma moeda no mapa
struct Moeda {
    int x, y;
    bool ativa;
};

// Array estático com as posições das moedas
Moeda moedasMapa[] = {
    {4, 0, false},
    {6, 2, true},
    {7, 13, true},
	{13, 11, false}
};
const int NUM_MOEDAS = sizeof(moedasMapa) / sizeof(Moeda);


vector <Tile> tileset;
vec2 pos; //armazena o indice i e j de onde o "personagem" está na cena

// --- VARIÁVEIS DE ESTADO DO JOGO ---
bool jogo_pausado = false;
bool jogador_ganhou = false;
bool jogador_perdeu = false;
double tempo_final = 0.0; // Tempo congelado ao finalizar o jogo
double tempo_inicio = 0.0; // Novo: tempo de início do jogo

void resetarJogo() {
    // Resetar mapas e barreiras
    leMapa("../assets/maps/mapa.txt", map);
    leMapa("../assets/maps/barreiras.txt", barreiras);
    // Resetar personagem
    pos.x = 0; pos.y = 0;
    migore.direcao = 3;
    migore.frame = 0;
    migore.andando = false;
    migore.grupoAnimacao = 0;
    // Resetar moedas
    moedasMapa[0].x = 4; moedasMapa[0].y = 0; moedasMapa[0].ativa = false;
    moedasMapa[1].x = 6; moedasMapa[1].y = 2; moedasMapa[1].ativa = true;
    moedasMapa[2].x = 7; moedasMapa[2].y = 13; moedasMapa[2].ativa = true;
    moedasMapa[3].x = 13; moedasMapa[3].y = 11; moedasMapa[3].ativa = false;
    // Resetar eventos
    evento_38_ativado = false;
    evento_210_ativado = false;
    evento_45_ativado = false;
    evento_512_ativado = false;
    evento_82_ativado = false;
    evento_83_ativado = false;
    evento_71_ativado = false;
    evento_41_ativado = false;
    evento_118_ativado = false;
    evento_1312_ativado = false;
    evento_jare_capturado = false;
    greatJareSpirit_ativo = false;
    pontuacao = 0;
    vidas = 5;
    animacoesTile.clear();
    jogo_pausado = false;
    jogador_ganhou = false;
    jogador_perdeu = false;
    tempo_final = 0.0; // Reseta o tempo congelado
    tempo_inicio = glfwGetTime(); // Novo: reseta o tempo de início
};

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Preenche o mapa a partir do arquivo
	leMapa("../assets/maps/mapa.txt", map);
	leMapa("../assets/maps/barreiras.txt", barreiras); // Lê o novo mapa de barreiras

	// Debug: imprime os mapas lidos
	std::cout << "Mapa principal:" << std::endl;
	imprimeMapa(map);
	std::cout << "Mapa de barreiras:" << std::endl;
	imprimeMapa(barreiras);

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Ativa a suavização de serrilhado (MSAA) com 8 amostras por pixel
	glfwWindowHint(GLFW_SAMPLES, 8);

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Triangulo! -- Rossana", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Falha ao inicializar GLAD" << std::endl;
		return -1;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	//Carregando uma textura 
	int imgWidth, imgHeight;
	//GLuint texID = loadTexture("../assets/sprites/Vampires1_Walk_full.png",imgWidth,imgHeight);
	GLuint texID = loadTexture("../assets/tilesets/tilesetIso.png",imgWidth,imgHeight);
	// Gerando um buffer simples, com a geometria de um triângulo
	/* Sprite vampirao;
	vampirao.nAnimations = 4;
	vampirao.nFrames = 6;
	vampirao.VAO = setupSprite(vampirao.nAnimations,vampirao.nFrames,vampirao.ds,vampirao.dt);
	vampirao.position = vec3(400.0, 150.0, 0.0);
	vampirao.dimensions = vec3(imgWidth/vampirao.nFrames*4,imgHeight/vampirao.nAnimations*4,1.0);
	vampirao.texID = texID;
	vampirao.iAnimation = 1;
	vampirao.iFrame = 0; */

	// Configura o tileset - conjunto de tiles do mapa
	for (int i=0; i < 7; i++)
	{
		Tile tile;
		tile.dimensions = vec3(100,50,1.0); // Tiles maiores para melhor visualização
		tile.iTile = i;
		tile.texID = texID;
		tile.VAO = setupTile(7,tile.ds,tile.dt);
		tile.caminhavel = true;
		tileset.push_back(tile);
	}

	// tileset[4].caminhavel = false; // Removido, pois o sistema usa apenas o mapa de barreiras

	// Inicializar a posição do "personagem"
	pos.x = 0;
	pos.y = 0;

	// Carrega o sprite animado do Migoré
	int migoreWidth, migoreHeight;
	migore.nDirecoes = 8;
	migore.nFrames = 4;
	migore.texID = loadTexture("../assets/tilesets/migore.png", migoreWidth, migoreHeight);
	migore.dimensions = vec3(100, 100, 1.0);
	migore.ds = 1.0f / migore.nFrames;
	migore.dt = 1.0f / migore.nDirecoes;
	migore.VAO = setupSprite(migore.nDirecoes, migore.nFrames, migore.ds, migore.dt);
	migore.direcao = 3; // Inicializa olhando para SO (Sudoeste)
	migore.frame = 0;

	// Carrega o sprite das moedas
	int moedasWidth, moedasHeight;
	moedas.nAnimations = 1;
	moedas.nFrames = 1; // Agora moedas são estáticas
	moedas.texID = loadTexture("../assets/sprites/moedas.png", moedasWidth, moedasHeight);
	moedas.dimensions = vec3(64, 64, 1.0); // Tamanho anterior
	moedas.ds = 1.0f; // Apenas um frame
	moedas.dt = 1.0f;
	moedas.VAO = setupSprite(moedas.nAnimations, moedas.nFrames, moedas.ds, moedas.dt);
	moedas.iAnimation = 0;
	moedas.iFrame = 0;

	// Carrega o sprite do great_jare_spirit (sem animação)
	int jareWidth, jareHeight;
	greatJareSpirit.nAnimations = 1;
	greatJareSpirit.nFrames = 1;
	greatJareSpirit.texID = loadTexture("../assets/sprites/great_jare_spirit.png", jareWidth, jareHeight);
	greatJareSpirit.dimensions = vec3(128, 128, 1.0); // Tamanho ajustável
	greatJareSpirit.ds = 1.0f;
	greatJareSpirit.dt = 1.0f;
	greatJareSpirit.VAO = setupSprite(greatJareSpirit.nAnimations, greatJareSpirit.nFrames, greatJareSpirit.ds, greatJareSpirit.dt);
	greatJareSpirit.iAnimation = 0;
	greatJareSpirit.iFrame = 0;

	// Carrega o sprite da top_bar
	int topBarWidth, topBarHeight;
	topBar.nAnimations = 1;
	topBar.nFrames = 1;
	topBar.texID = loadTexture("../assets/sprites/top_bar.png", topBarWidth, topBarHeight);
	topBar.dimensions = vec3(topBarWidth, topBarHeight, 1.0);
	topBar.ds = 1.0f;
	topBar.dt = 1.0f;
	topBar.VAO = setupSprite(topBar.nAnimations, topBar.nFrames, topBar.ds, topBar.dt);
	topBar.iAnimation = 0;
	topBar.iFrame = 0;

	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	double prev_s = glfwGetTime();	// Define o "tempo anterior" inicial.
	double title_countdown_s = 0.1; // Intervalo para atualizar o título da janela com o FPS.

	float colorValue = 0.0;

	// Ativando o primeiro buffer de textura do OpenGL
	glActiveTexture(GL_TEXTURE0);

	// Criando a variável uniform pra mandar a textura pro shader
	glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

	// Matriz de projeção paralela ortográfica
	mat4 projection = ortho(0.0, 960.0, 720.0, 0.0, -1.0, 1.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	glEnable(GL_DEPTH_TEST); // Habilita o teste de profundidade
	glDepthFunc(GL_ALWAYS); // Testa a cada ciclo

	glEnable(GL_BLEND); //Habilita a transparência -- canal alpha
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Seta função de transparência


	double lastTime = 0.0;
	double deltaT = 0.0;
	double currTime = glfwGetTime();
	double FPS = 12.0;
	

	// Loop da aplicação - "game loop"
    static double prev_anim_s = glfwGetTime(); // Corrigido: fora do if
	while (!glfwWindowShouldClose(window))
	{
		// Checa eventos de input
		glfwPollEvents();

		// --- PAUSA: impede atualização do jogo se pausado ---
        if (jogo_pausado) {
            // Limpa tela
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // --- DESENHA TOP BAR COMO FUNDO DO HUD (igual ao modo normal) ---
            {
                float x = ((topBar.dimensions.x) / 3 )* 2;
                float y = 30.0f; // Topo da janela
                mat4 model = mat4(1);
                model = translate(model, vec3(x, y, 0.0f));
                model = scale(model, topBar.dimensions);
                glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
                vec2 offsetTex;
                offsetTex.s = topBar.iFrame * topBar.ds;
                offsetTex.t = topBar.iAnimation * topBar.dt;
                glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
                glBindVertexArray(topBar.VAO);
                glBindTexture(GL_TEXTURE_2D, topBar.texID);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            // --- HUD alinhado bonito ---
            char info[128];
            float y_hud = 25.0f;
            sprintf(info, "Pontos: %.0f", pontuacao);
            drawText_GL33(WIDTH/2 - 260, y_hud, info, 0.878f, 0.235f, 0.157f, 2.0f);
            sprintf(info, "Vidas: %d", vidas);
            drawText_GL33(WIDTH/2 - 40, y_hud, info, 0.878f, 0.235f, 0.157f, 2.0f);
            double tempo_jogo = tempo_final - tempo_inicio; // Corrigido
            sprintf(info, "Tempo: %.1fs", tempo_jogo);
            drawText_GL33(WIDTH/2 + 140, y_hud, info, 0.878f, 0.235f, 0.157f, 2.0f);
            // --- Mensagem de fim de jogo ---
            const char* msg = jogador_ganhou ? "VOCE GANHOU!" : "GAME OVER";
            // Centralização simples, igual ao HUD
            drawText_GL33(WIDTH/2-120, HEIGHT/2-40, msg, 1, 1, 0, 3.0f);
            drawText_GL33(WIDTH/2-190, HEIGHT/2+20, "Pressione ENTER para reiniciar", 1, 1, 1, 2.0f);
            glUseProgram(shaderID);
            glBindVertexArray(0);
            glfwSwapBuffers(window);
            continue;
        }

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        

		// Desenhar o mapa
		verificaEventoMapa(pos.x, pos.y);
		desenharMapa(shaderID);
		desenharPersonagem(shaderID);
		desenharMoedas(shaderID);
		desenharGreatJareSpirit(shaderID);


		// Atualiza animação do personagem
		currTime = glfwGetTime();
		deltaT += currTime - lastTime;
		lastTime = currTime;
		static int frameAnim = 0;
		if (migore.andando) {
			if (deltaT >= 1.0 / FPS) {
				frameAnim = (frameAnim + 1) % 2; // Só 2 frames por grupo
				migore.frame = migore.grupoAnimacao * 2 + frameAnim;
				deltaT = 0.0;
			}
		} else {
			migore.frame = 0;
			frameAnim = 0;
		}

		// --- Atualiza animações de troca de tile, se houver ---
        double curr_s = glfwGetTime();
        double elapsed = curr_s - prev_anim_s;
        prev_anim_s = curr_s;
        if (!animacoesTile.empty()) {
            for (auto &anim : animacoesTile) {
                anim.tempo += elapsed;
                // alterna o tile a cada 0.08s
                if (fmod(anim.tempo, 0.16) < 0.08) {
                    map[anim.x][anim.y] = anim.tileInicial;
                } else {
                    map[anim.x][anim.y] = anim.tileFinal;
                }
            }
            // Remove animações finalizadas
            animacoesTile.erase(
                std::remove_if(animacoesTile.begin(), animacoesTile.end(), [](AnimacaoTile &anim) {
                    if (anim.tempo >= anim.tempoTotal) {
                        map[anim.x][anim.y] = anim.tileFinal;
                        return true;
                    }
                    return false;
                }),
                animacoesTile.end()
            );
        }

		// --- CHECA CONDIÇÃO DE VITÓRIA/DERROTA ---
        // Vitória: chegou ao final (exemplo: tile 14,5)
        if (!jogo_pausado && pos.x == 14 && pos.y == 5) {
            jogo_pausado = true;
            jogador_ganhou = true;
            jogador_perdeu = false;
            tempo_final = glfwGetTime(); // Congela o tempo
        }
        // Derrota: ficou sem vidas
        if (!jogo_pausado && vidas <= 0) {
            jogo_pausado = true;
            jogador_perdeu = true;
            jogador_ganhou = false;
            tempo_final = glfwGetTime(); // Congela o tempo
        }

		// --- DESENHA TOP BAR COMO FUNDO DO HUD ---
        {
            float x = ((topBar.dimensions.x) / 3 )* 2;
            float y = 30.0f; // Agora, topo da janela
            mat4 model = mat4(1);
            model = translate(model, vec3(x, y, 0.0f));
            model = scale(model, topBar.dimensions);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
            vec2 offsetTex;
            offsetTex.s = topBar.iFrame * topBar.ds;
            offsetTex.t = topBar.iAnimation * topBar.dt;
            glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
            glBindVertexArray(topBar.VAO);
            glBindTexture(GL_TEXTURE_2D, topBar.texID);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
		
        // --- DESENHO DE TEXTOS (HUD) ---
        char info[128];
        float y_hud = 25.0f;
        // Pontos à esquerda
        sprintf(info, "Pontos: %.0f", pontuacao);
        drawText_GL33(WIDTH/2 - 260, y_hud, info, 0.878f, 0.235f, 0.157f, 2.0f);
        // Vidas ao centro
        sprintf(info, "Vidas: %d", vidas);
        drawText_GL33(WIDTH/2 - 40, y_hud, info, 0.878f, 0.235f, 0.157f, 2.0f);
        // Tempo à direita
        double tempo_jogo = jogo_pausado ? (tempo_final - tempo_inicio) : (glfwGetTime() - tempo_inicio); // Corrigido
        sprintf(info, "Tempo: %.1fs", tempo_jogo);
        drawText_GL33(WIDTH/2 + 140, y_hud, info, 0.878f, 0.235f, 0.157f, 2.0f);
       

        // --- MENSAGEM DE FIM DE JOGO ---
        if (jogo_pausado) {
            const char* msg = jogador_ganhou ? "VOCE GANHOU!" : "GAME OVER";
            // Centralização simples, igual ao HUD
            drawText_GL33(WIDTH/2-180, HEIGHT/2-40, msg, 1, 1, 0, 3.0f);
            drawText_GL33(WIDTH/2-260, HEIGHT/2+20, "Pressione ENTER para reiniciar", 1, 1, 1, 2.0f);
        }
        // Garante que o shader do jogo está ativo após desenhar texto
        glUseProgram(shaderID);
        glBindVertexArray(0);
        // Troca os buffers da tela
		glfwSwapBuffers(window);
	}
		
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    // Permite fechar com ESC sempre
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // --- Se o jogo está pausado, só aceita ENTER para resetar ---
    if (jogo_pausado) {
        if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
            resetarJogo();
        }
        return;
    }

	vec2 aux = pos;

	migore.andando = false; // por padrão, parado

	if (key == GLFW_KEY_W && action == GLFW_PRESS) // NORTE
	{
		if (pos.x > 0) pos.x--;
		if (pos.y > 0) pos.y--;
		migore.direcao = 0; // N
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2; // sorteia grupo
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) // NOROESTE
	{
		if (pos.x > 0) pos.x--;
		migore.direcao = 1; // NO
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2;
	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS) // OESTE
	{
		if (pos.x > 0) pos.x--;
		if (pos.y <= TILEMAP_HEIGHT - 2) pos.y++;
		migore.direcao = 2; // O
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) // SUDOESTE
	{
		if (pos.y <= TILEMAP_HEIGHT - 2) pos.y++;
		migore.direcao = 3; // SO
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) // SUL
	{
		if (pos.x <= TILEMAP_WIDTH -2) pos.x++;
		if (pos.y <= TILEMAP_HEIGHT - 2) pos.y++;
		migore.direcao = 4; // S
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2;
	}
	if (key == GLFW_KEY_C && action == GLFW_PRESS) // SUDESTE
	{
		if (pos.x <= TILEMAP_WIDTH -2) pos.x++;
		migore.direcao = 5; // SE
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS) // LESTE
	{
		if (pos.x <= TILEMAP_WIDTH -2) pos.x++;
		if (pos.y > 0) pos.y--;
		migore.direcao = 6; // L
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2;
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS) // NORDESTE
	{
		if (pos.y > 0) pos.y--;
		migore.direcao = 7; // NE
		migore.andando = true;
		migore.grupoAnimacao = rand() % 2;
	}
	// Nova lógica de colisão: só permite andar se barreiras[x][y] == 0
	if (barreiras[(int)pos.x][(int)pos.y] != 0)
	{
		pos = aux; //recebe a pos não mudada :P
	}

	cout << "(" << pos.x <<"," << pos.y << ")" << endl;

	
}

// Função para verificar eventos do tile atual do mapa (lava, moedas, etc)
void verificaEventoMapa(int posx, int posy) {

	// --------------------- LÓGICA DA LAVA
    if (map[posx][posy] == 3) {
        std::cout << "Você morreu! Caiu na lava!" << std::endl;
        pos.x = 0;
        pos.y = 0;
        vidas--;
    }

	// --------------------- LÓGICA DAS MOEDAS
	if(posx == 4 && posy == 0 && moedasMapa[0].ativa) {
		coletarMoeda(0);
	}
	if(posx == 6 && posy == 2 && moedasMapa[1].ativa) {
		coletarMoeda(1);
	}
	if(posx == 7 && posy == 13 && moedasMapa[2].ativa) {
		coletarMoeda(2);
	}
	if(posx == 13 && posy == 11 && moedasMapa[3].ativa) {
		coletarMoeda(3);
	}

	// --------------------- LÓGICA DO GREAT JARE SPIRIT
	if (posx == 12 && posy == 11 && greatJareSpirit_ativo && !evento_jare_capturado) {
		std::cout << "Você encontrou o Great Jare Spirit!" << std::endl;
		animarTrocaTile(12, 11, 5);
		greatJareSpirit_ativo = false;
		evento_jare_capturado = true;
		pontuacao *= 1.25f;
		
	}

	// --------------------- LOGICAS DOS BOTOES
    // Primeiro botão
    if (posx == 3 && posy == 7 && !evento_38_ativado) {
        liberarTileComAnimacao(2, 8, evento_38_ativado);
    }
    // Segundo botão
    if (posx == 1 && posy == 9 && !evento_210_ativado) {
        liberarTileComAnimacao(2, 10, evento_210_ativado);
    }
    // Terceiro botão
    if (posx == 2 && posy == 13 && !evento_45_ativado) {
        liberarTileComAnimacao(4, 5, evento_45_ativado);
    }
    // Quarto botão
    if (posx == 5 && posy == 11 && !evento_512_ativado) {
        liberarTileComAnimacao(5, 12, evento_512_ativado);
    }
    // Quinto botão
    if (posx == 9 && posy == 13 && (!evento_82_ativado)) {
        liberarTileComAnimacao(8, 2, evento_82_ativado);
		liberarTileComAnimacao(8, 3, evento_83_ativado);
    }
    // Sexto botão
    if (posx == 13 && posy == 1 && !evento_71_ativado) {
        liberarTileComAnimacao(7, 1, evento_71_ativado);
    }
    // Sétimo botão
    if (posx == 5 && posy == 1 && !evento_41_ativado) {
        liberarTileComAnimacao(4, 1, evento_41_ativado);
		moedasMapa[0].ativa = true; // Ativa a moeda que está escondida
    }
    // Oitavo botão
    if (posx == 5 && posy == 2 && !evento_118_ativado) {
        liberarTileComAnimacao(11, 8, evento_118_ativado);
    }
    // Nono botão
    if (posx == 14 && posy == 3 && !evento_1312_ativado) {
        liberarTileComAnimacao(12, 10, evento_1312_ativado);
		// barreiras[12][12] = 1; // Bloqueia a coordenada 12,12
		// barreiras[13][12] = 1; // Bloqueia a coordenada 13,12
		moedasMapa[3].ativa = true; // Ativa a moeda final
		greatJareSpirit_ativo = true; // Ativa o sprite do Great Jare Spirit
    }

    
}

// Função utilitária para imprimir o mapa no console PARA DEBUG
void imprimeMapa(int map[][TILEMAP_WIDTH]) {
    for (int y = 0; y < TILEMAP_HEIGHT; y++) {
        for (int x = 0; x < TILEMAP_WIDTH; x++) {
            std::cout << map[x][y];
            if (x < TILEMAP_WIDTH - 1)
                std::cout << ",";
        }
        std::cout << std::endl;
    }
}

// Esta função está bastante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupSprite(int nAnimations, int nFrames, float &ds, float &dt)
{

	ds = 1.0 / (float) nFrames;
	dt = 1.0 / (float) nAnimations;
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {
		// x   y    z    s     t
		-0.5,  0.5, 0.0, 0.0, dt, //V0
		-0.5, -0.5, 0.0, 0.0, 0.0, //V1
		 0.5,  0.5, 0.0, ds, dt, //V2
		 0.5, -0.5, 0.0, ds, 0.0  //V3
		};

	GLuint VBO, VAO;
	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
	//  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	//  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
	//  Tipo do dado
	//  Se está normalizado (entre zero e um)
	//  Tamanho em bytes
	//  Deslocamento a partir do byte zero

	// Ponteiro pro atributo 0 - Posição - coordenadas x, y, z
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Ponteiro pro atributo 1 - Coordenada de textura s, t
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

int setupTile(int nTiles, float &ds, float &dt)
{
    
	ds = 1.0 / (float) nTiles;
	dt = 1.0;
	
	// Como eu prefiro escalar depois, th e tw serão 1.0
	float th = 1.0, tw = 1.0;

	GLfloat vertices[] = {
		// x   y    z    s     t
		0.0,  th/2.0f,   0.0, 0.0,    dt/2.0f, //A
		tw/2.0f, th,     0.0, ds/2.0f, dt,     //B
		tw/2.0f, 0.0,    0.0, ds/2.0f, 0.0,    //D
		tw,     th/2.0f, 0.0, ds,     dt/2.0f  //C
		};

	GLuint VBO, VAO;
	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
	//  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	//  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
	//  Tipo do dado
	//  Se está normalizado (entre zero e um)
	//  Tamanho em bytes
	//  Deslocamento a partir do byte zero

	// Ponteiro pro atributo 0 - Posição - coordenadas x, y, z
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Ponteiro pro atributo 1 - Coordenada de textura s, t
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

int loadTexture(string filePath, int &width, int &height)
{
	GLuint texID;

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int nrChannels;

	// Corrige a orientação vertical das imagens (OpenGL espera origem no canto inferior esquerdo)
	stbi_set_flip_vertically_on_load(true);

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

void leMapa(const std::string& path, int map[][TILEMAP_WIDTH]) {
    std::ifstream arquivo(path);
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << path << std::endl;
        return;
    }
    std::string linha;
    int y = 0;
    while (std::getline(arquivo, linha) && y < TILEMAP_HEIGHT) {
        int x = 0;
        size_t start = 0, end;
        while (x < TILEMAP_WIDTH && (end = linha.find(',', start)) != std::string::npos) {
            std::string valor = linha.substr(start, end - start);
            if (!valor.empty() && valor.find_first_not_of(" \t\n\r") != std::string::npos)
                map[x++][y] = std::stoi(valor);
            start = end + 1;
        }
        // Último valor da linha (após a última vírgula)
        if (x < TILEMAP_WIDTH && start < linha.size()) {
            std::string valor = linha.substr(start);
            if (!valor.empty() && valor.find_first_not_of(" \t\n\r") != std::string::npos)
                map[x++][y] = std::stoi(valor);
        }
        y++;
    }
}

void desenharMapa(GLuint shaderID)
{
	// Calcula o centro da tela
	float tela_cx = WIDTH / 2.0f;
	float tela_cy = HEIGHT / 2.0f;

	// Tile do personagem
	Tile tile_central = tileset[6];
	float tile_w = tile_central.dimensions.x;
	float tile_h = tile_central.dimensions.y;

	// Offset para centralizar o personagem
	float x0 = tela_cx - (pos.x - pos.y) * tile_w / 2.0f - tile_w / 2.0f; // desloca meio tile para a esquerda
	float y0 = tela_cy - (pos.x + pos.y) * tile_h / 2.0f;

	for(int y=0; y<TILEMAP_HEIGHT; y++)
	{
		for (int x=0; x < TILEMAP_WIDTH; x++)
		{
			mat4 model = mat4(1);
			Tile curr_tile = tileset[map[x][y]];

			float draw_x = x0 + (x-y) * curr_tile.dimensions.x/2.0;
			float draw_y = y0 + (x+y) * curr_tile.dimensions.y/2.0;

			model = translate(model, vec3(draw_x,draw_y,0.0));
			model = scale(model,curr_tile.dimensions);
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

			vec2 offsetTex;
			offsetTex.s = curr_tile.iTile * curr_tile.ds;
			offsetTex.t = 0.0;
			glUniform2f(glGetUniformLocation(shaderID, "offsetTex"),offsetTex.s, offsetTex.t);

			glBindVertexArray(curr_tile.VAO);
			glBindTexture(GL_TEXTURE_2D, curr_tile.texID);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}
}

void desenharPersonagem(GLuint shaderID)
{
	// Desenha o Migoré animado no centro do tile onde ele está
	float tela_cx = WIDTH / 2.0f;
	float tela_cy = HEIGHT / 2.0f;
	Tile tile_central = tileset[6];
	float tile_w = tile_central.dimensions.x;
	float tile_h = tile_central.dimensions.y;
	float x0 = tela_cx - (pos.x - pos.y) * tile_w / 2.0f;
	float y0 = tela_cy - (pos.x + pos.y) * tile_h / 2.0f;
	float x = x0 + (pos.x - pos.y) * tile_w / 2.0f;
	float y = y0 + (pos.x + pos.y) * tile_h / 2.0f;

	mat4 model = mat4(1);
	model = translate(model, vec3(x, y, 0.0));
	model = scale(model, migore.dimensions);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	vec2 offsetTex;
	offsetTex.s = migore.frame * migore.ds;
	offsetTex.t = migore.direcao * migore.dt;
	glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);

	glBindVertexArray(migore.VAO);
	glBindTexture(GL_TEXTURE_2D, migore.texID);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void desenharMoedas(GLuint shaderID) {
    float tela_cx = WIDTH / 2.0f;
    float tela_cy = HEIGHT / 2.0f;
    Tile tile_central = tileset[6];
    float tile_w = tile_central.dimensions.x;
    float tile_h = tile_central.dimensions.y;
    float x0 = tela_cx - (pos.x - pos.y) * tile_w / 2.0f;
    float y0 = tela_cy - (pos.x + pos.y) * tile_h / 2.0f;

    double tempo = glfwGetTime();
    float amplitude = 12.0f; // altura máxima do pulo
    float freq = 1.5f; // velocidade da flutuação

    for (int i = 0; i < NUM_MOEDAS; i++) {
        if (!moedasMapa[i].ativa) continue;
        int j = moedasMapa[i].x;
        int k = moedasMapa[i].y;
        float x = x0 + (j - k) * tile_w / 2.0f;
        float y = y0 + (j + k) * tile_h / 2.0f;
        // Aplica deslocamento vertical animado
        float offsetY = amplitude * sinf((float)tempo * freq + i);
        mat4 model = mat4(1);
        model = translate(model, vec3(x, y + offsetY, 0.0));
        model = scale(model, moedas.dimensions);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
        vec2 offsetTex;
        offsetTex.s = moedas.iFrame * moedas.ds;
        offsetTex.t = moedas.iAnimation * moedas.dt;
        glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
        glBindVertexArray(moedas.VAO);
        glBindTexture(GL_TEXTURE_2D, moedas.texID);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

// Função para desenhar o great_jare_spirit
void desenharGreatJareSpirit(GLuint shaderID) {
    if (!greatJareSpirit_ativo) return;
    // Desenha centralizando o centro do sprite no centro do tile (13,11)
    Tile tile_central = tileset[6];
    float tile_w = tile_central.dimensions.x;
    float tile_h = tile_central.dimensions.y;
    float tela_cx = WIDTH / 2.0f;
    float tela_cy = HEIGHT / 2.0f;
    float x0 = tela_cx - (pos.x - pos.y) * tile_w / 2.0f - tile_w / 2.0f;
    float y0 = tela_cy - (pos.x + pos.y) * tile_h / 2.0f;
    int j = 13, k = 11;
    float x = x0 + (j - k) * tile_w / 2.0f;
    float y = y0 + (j + k) * tile_h / 2.0f;
    // Centraliza o sprite no tile
    x = x + tile_w / 2.0f - greatJareSpirit.dimensions.x / 2.0f;
    y = y + tile_h / 2.0f - greatJareSpirit.dimensions.y / 2.0f;
    mat4 model = mat4(1);
    model = translate(model, vec3(x, y, 0.0));
    model = scale(model, greatJareSpirit.dimensions);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
    vec2 offsetTex;
    offsetTex.s = 0.0f;
    offsetTex.t = 0.0f;
    glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);
    glBindVertexArray(greatJareSpirit.VAO);
    glBindTexture(GL_TEXTURE_2D, greatJareSpirit.texID);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Função genérica para animar e liberar tile
void liberarTileComAnimacao(int x, int y, bool &evento_ativado) {
    if (!evento_ativado) {
        animarTrocaTile(x, y, 0); // anima para chão
        barreiras[x][y] = 0; // libera barreira
        evento_ativado = true;
        cout << "[LOG] Evento especial em (" << x << "," << y << "): ANIMANDO tile para liberar!" << std::endl;
    }
}

// Função para coletar moeda
void coletarMoeda(int indice) {
    moedasMapa[indice].ativa = false;
    pontuacao += 340;
    std::cout << "Você coletou uma moeda! Pontuação: " << pontuacao << std::endl;
    animarTrocaTile(moedasMapa[indice].x, moedasMapa[indice].y, 5);
}

#include "../../include/glad/stb_easy_font.h"

// Função utilitária para desenhar texto usando stb_easy_font e OpenGL moderno
void drawText_GL33(float x, float y, const char* text, float r, float g, float b, float scale) {
    static GLuint vbo = 0, vao = 0;
    static GLuint shader = 0;
    static GLint uniColor = -1, uniProjection = -1;

    char buffer[99999];
    int num_quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer));

    // Aplica escala nos vértices
    float* verts = (float*)buffer;
    for (int i = 0; i < num_quads * 4; ++i) {
        verts[i * 4 + 0] = verts[i * 4 + 0] * scale + x;
        verts[i * 4 + 1] = verts[i * 4 + 1] * scale + y;
    }

    if (shader == 0) {
        // Vertex shader simples para 2D
        const char* vs =
            "#version 330 core\n"
            "layout(location=0) in vec2 pos;\n"
            "uniform mat4 projection;\n"
            "void main(){ gl_Position = projection * vec4(pos,0,1); }\n";
        // Fragment shader simples para cor uniforme
        const char* fs =
            "#version 330 core\n"
            "uniform vec3 color;\n"
            "out vec4 FragColor;\n"
            "void main(){ FragColor = vec4(color,1); }\n";
        GLuint vsId = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsId = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(vsId, 1, &vs, NULL); glCompileShader(vsId);
        glShaderSource(fsId, 1, &fs, NULL); glCompileShader(fsId);
        shader = glCreateProgram();
        glAttachShader(shader, vsId); glAttachShader(shader, fsId);
        glLinkProgram(shader);
        glDeleteShader(vsId); glDeleteShader(fsId);
        uniColor = glGetUniformLocation(shader, "color");
        uniProjection = glGetUniformLocation(shader, "projection");
    }

    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, num_quads * 4 * 16, buffer, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void*)0);

    glUseProgram(shader);

    // Projeção ortográfica para 2D
    float ortho[16] = {
        2.0f/WIDTH, 0, 0, 0,
        0, -2.0f/HEIGHT, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fv(uniProjection, 1, GL_FALSE, ortho);
    glUniform3f(uniColor, r, g, b);

    glDrawArrays(GL_QUADS, 0, num_quads * 4);

    glBindVertexArray(0);
    glUseProgram(0);
}
