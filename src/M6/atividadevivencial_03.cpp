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
	//int iAnimation, iFrame;
	//int nAnimations, nFrames;
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

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

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


 #define TILEMAP_WIDTH 10
 #define TILEMAP_HEIGHT 10
int map[10][10] = {
    0, 0, 0, 1, 1, 1, 2, 2, 2, 2,
    0, 4, 0, 1, 4, 1, 2, 4, 2, 2,
    0, 4, 0, 1, 4, 1, 2, 4, 2, 2,
    0, 0, 0, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 5, 5, 5, 0, 0, 0, 0,
    3, 4, 3, 5, 4, 5, 0, 4, 0, 0,
    3, 4, 3, 5, 4, 5, 0, 4, 0, 0,
    3, 3, 3, 5, 5, 5, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 2, 2, 2, 2,
    0, 0, 0, 1, 1, 1, 2, 2, 2, 2
};

vector <Tile> tileset;

vec2 pos; //armazena o indice i e j de onde o "personagem" está na cena
// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

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
		tile.dimensions = vec3(114,57,1.0);
		tile.iTile = i;
		tile.texID = texID;
		tile.VAO = setupTile(7,tile.ds,tile.dt);
		tile.caminhavel = true;
		tileset.push_back(tile);
	}

	tileset[4].caminhavel = false; //agua

	// Inicializar a posição do "personagem"
	pos.x = 0;
	pos.y = 0;



	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	double prev_s = glfwGetTime();	// Define o "tempo anterior" inicial.
	double title_countdown_s = 0.1; // Intervalo para atualizar o título da janela com o FPS.

	float colorValue = 0.0;

	// Ativando o primeiro buffer de textura do OpenGL
	glActiveTexture(GL_TEXTURE0);

	// Criando a variável uniform pra mandar a textura pro shader
	glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

	// Matriz de projeção paralela ortográfica
	mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
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
	while (!glfwWindowShouldClose(window))
	{
		// Este trecho de código é totalmente opcional: calcula e mostra a contagem do FPS na barra de título
		{
			double curr_s = glfwGetTime();		// Obtém o tempo atual.
			double elapsed_s = curr_s - prev_s; // Calcula o tempo decorrido desde o último frame.
			prev_s = curr_s;					// Atualiza o "tempo anterior" para o próximo frame.

			// Exibe o FPS, mas não a cada frame, para evitar oscilações excessivas.
			title_countdown_s -= elapsed_s;
			if (title_countdown_s <= 0.0 && elapsed_s > 0.0)
			{
				double fps = 1.0 / elapsed_s; // Calcula o FPS com base no tempo decorrido.

				// Cria uma string e define o FPS como título da janela.
				char tmp[256];
				sprintf(tmp, "Ola Triangulo! -- Rossana\tFPS %.2lf", fps);
				glfwSetWindowTitle(window, tmp);

				title_countdown_s = 0.1; // Reinicia o temporizador para atualizar o título periodicamente.
			}
		}

		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		// Desenhar o mapa
		desenharMapa(shaderID);
		desenharPersonagem(shaderID);

		//---------------------------------------------------------------------
		// Desenho do vampirao
		// Matriz de transformaçao do objeto - Matriz de modelo
		/* model = mat4(1); //matriz identidade
		model = translate(model,vampirao.position);
		model = rotate(model, radians(0.0f), vec3(0.0, 0.0, 1.0));
		model = scale(model,vampirao.dimensions);
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

		vec2 offsetTex;

		if (deltaT >= 1.0/FPS)
		{
			vampirao.iFrame = (vampirao.iFrame + 1) % vampirao.nFrames; // incremento "circular"
			lastTime = currTime;
		}

		offsetTex.s = vampirao.iFrame * vampirao.ds;
		offsetTex.t = 0.0;
		glUniform2f(glGetUniformLocation(shaderID, "offsetTex"),offsetTex.s, offsetTex.t);

		glBindVertexArray(vampirao.VAO); // Conectando ao buffer de geometria
		glBindTexture(GL_TEXTURE_2D, vampirao.texID); // Conectando ao buffer de textura

		// Chamada de desenho - drawcall
		// Poligono Preenchido - GL_TRIANGLES
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); */
		//---------------------------------------------------------------------------

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
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	vec2 aux = pos;

	if (key == GLFW_KEY_W && action == GLFW_PRESS) // NORTE
	{
		if (pos.x > 0)
		{
			pos.x--;
		}

		if (pos.y > 0)
		{
			pos.y--;
		}


	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS) // OESTE
	{
		if (pos.x > 0)
		{
			pos.x--;
		}
		if (pos.y <= TILEMAP_HEIGHT - 2)
		{
			pos.y++;
		}


	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) // SUL
	{
		if (pos.x <= TILEMAP_WIDTH -2)
		{
			pos.x++;
		}

		if (pos.y <= TILEMAP_HEIGHT - 2)
		{
			pos.y++;
		}

	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS) // LESTE
	{
		if (pos.x <= TILEMAP_WIDTH -2)
		{
			pos.x++;
		}

		if (pos.y > 0)
		{
			pos.y--;
		}

	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) //NOROESTE
	{
		if (pos.x > 0)
		{
			pos.x--;
		}

	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS) //NORDESTE
	{
		if (pos.y > 0)
		{
			pos.y--;
		}

	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) //SUDOESTE
	{
		if (pos.y <= TILEMAP_HEIGHT - 2)
		{
			pos.y++;
		}

	}
	if (key == GLFW_KEY_X && action == GLFW_PRESS) //SUDESTE
	{
		if (pos.x <= TILEMAP_WIDTH -2)
		{
			pos.x++;
		}

	}

	if (!tileset[map[(int)pos.y][(int)pos.x]].caminhavel)
	{
		pos = aux; //recebe a pos não mudada :P
	}

	cout << "(" << pos.x <<"," << pos.y << ")" << endl;

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

void desenharMapa(GLuint shaderID)
{
	//dá pra fazer um cálculo usando tilemap_width e tilemap_height
	float x0 = 400;
	float y0 = 100;

	for(int i=0; i<TILEMAP_HEIGHT; i++)
	{
		for (int j=0; j < TILEMAP_WIDTH; j++)
		{
			// Matriz de transformaçao do objeto - Matriz de modelo
			mat4 model = mat4(1); //matriz identidade
			
			Tile curr_tile = tileset[map[i][j]];

			float x = x0 + (j-i) * curr_tile.dimensions.x/2.0;
			float y = y0 + (j+i) * curr_tile.dimensions.y/2.0;

			model = translate(model, vec3(x,y,0.0));
			model = scale(model,curr_tile.dimensions);
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

		vec2 offsetTex;

		offsetTex.s = curr_tile.iTile * curr_tile.ds;
		offsetTex.t = 0.0;
		glUniform2f(glGetUniformLocation(shaderID, "offsetTex"),offsetTex.s, offsetTex.t);

		glBindVertexArray(curr_tile.VAO); // Conectando ao buffer de geometria
		glBindTexture(GL_TEXTURE_2D, curr_tile.texID); // Conectando ao buffer de textura

		// Chamada de desenho - drawcall
		// Poligono Preenchido - GL_TRIANGLES
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); 
		}
	}
}

void desenharPersonagem(GLuint shaderID)
{
	Tile curr_tile = tileset[6]; //tile rosa

	float x0 = 400;
	float y0 = 100;

	float x = x0 + (pos.x-pos.y) * curr_tile.dimensions.x/2.0;
	float y = y0 + (pos.x+pos.y) * curr_tile.dimensions.y/2.0;

	mat4 model = mat4(1);
	model = translate(model, vec3(x,y,0.0));
	model = scale(model,curr_tile.dimensions);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	vec2 offsetTex;

	offsetTex.s = curr_tile.iTile * curr_tile.ds;
	offsetTex.t = 0.0;
	glUniform2f(glGetUniformLocation(shaderID, "offsetTex"),offsetTex.s, offsetTex.t);

	glBindVertexArray(curr_tile.VAO); // Conectando ao buffer de geometria
	glBindTexture(GL_TEXTURE_2D, curr_tile.texID); // Conectando ao buffer de textura

	// Chamada de desenho - drawcall
	// Poligono Preenchido - GL_TRIANGLES
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
