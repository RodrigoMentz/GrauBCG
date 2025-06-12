/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 13/08/2024
 *
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

#include <cmath>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
GLuint loadTexture(string filePath, int &width, int &height);
int loadSimpleOBJ(string filePATH, int &nVertices);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texc;
layout (location = 3) in vec3 normal;

uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;
void main()
{
   	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
	fragPos = model * vec4(position.x, position.y, position.z, 1.0);
	texCoord = vec2(texc.s, 1 - texc.t);
	vNormal = normal;
	vColor = vec4(color,1.0);
})";

const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
uniform vec3 lightPos;
uniform vec3 camPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;
out vec4 color;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;
void main()
{

	vec3 lightColor = vec3(1.0,1.0,1.0);
	vec4 objectColor = texture(texBuff,texCoord);
	//vec4 objectColor = vColor;

	//Coeficiente de luz ambiente
	vec3 ambient = ka * lightColor;

	//Coeficiente de reflexão difusa
	vec3 N = normalize(vNormal);
	vec3 L = normalize(lightPos - vec3(fragPos));
	float diff = max(dot(N, L),0.0);
	vec3 diffuse = kd * diff * lightColor;

	//Coeficiente de reflexão especular
	vec3 R = normalize(reflect(-L,N));
	vec3 V = normalize(camPos - vec3(fragPos));
	float spec = max(dot(R,V),0.0);
	spec = pow(spec,q);
	vec3 specular = ks * spec * lightColor; 

	vec3 result = (ambient + diffuse) * vec3(objectColor) + specular;
	color = vec4(result,1.0);

})";

struct Material {
	glm::vec3 ka;
	glm::vec3 kd;
	glm::vec3 ks;
	std::string textureFile;
};

struct Object
{
	GLuint VAO; //Índice do buffer de geometria
	GLuint texID; //Identificador da textura carregada
	int nVertices; //nro de vértices
	glm::mat4 model;
	float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
	float tamanhoEscala = 1.0f;
	bool rotateX=false, rotateY=false, rotateZ=false;
	Material material;
	int TextureimgWidth, TextureimgHeight;
};

std::unordered_map<std::string, Material> materiais;
std::string nomeMaterial;
// Object obj;
// Object obj2;

Object objs[2];

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio M4 - Rodrigo Korte Mentz", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
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
    
	objs[0].VAO = loadSimpleOBJ("../assets/Modelos3D/SuzanneSubdiv1.obj", objs[0].nVertices);
	objs[0].material = materiais["Material.001"];
	objs[0].texID = loadTexture("../assets/Modelos3D/" + objs[0].material.textureFile, objs[0].TextureimgWidth,objs[0].TextureimgHeight);

	objs[1].VAO = loadSimpleOBJ("../assets/Modelos3D/SuzanneSubdiv1.obj", objs[1].nVertices);
	objs[1].material = materiais["Material.001"];
	objs[1].texID = loadTexture("../assets/Modelos3D/" + objs[1].material.textureFile, objs[1].TextureimgWidth,objs[1].TextureimgHeight);
	objs[1].rotateX = true;
	objs[1].posX = 3.0f;

    float q = 10.0;
    vec3 lightPos = vec3(0.6, 1.2, -0.5);
	vec3 camPos = vec3(0.0,0.0,-3.0);

	glUseProgram(shaderID);

	// Enviar a informação de qual variável armazenará o buffer da textura
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    // glUniform1f(glGetUniformLocation(shaderID, "ka"), mat.ka.r);
	// glUniform1f(glGetUniformLocation(shaderID, "kd"), mat.kd.r);
	// glUniform1f(glGetUniformLocation(shaderID, "ks"), mat.ks.r);
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);
	glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x,lightPos.y,lightPos.z);
	glUniform3f(glGetUniformLocation(shaderID, "camPos"), camPos.x,camPos.y,camPos.z);

	//Ativando o primeiro buffer de textura da OpenGL
	glActiveTexture(GL_TEXTURE0);
	

	// Matriz de projeção paralela ortográfica
	mat4 projection = ortho(-5.0, 5.0, -5.0, 5.0, -5.0, 5.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	// Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); // matriz identidade
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	glEnable(GL_DEPTH_TEST);


	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float angle = (GLfloat)glfwGetTime();
		for (Object& obj : objs) {
			obj.model = glm::mat4(1.0f);
			if (obj.rotateX)
			{
				obj.model = glm::rotate(obj.model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
			}
			else if (obj.rotateY)
			{
				obj.model = glm::rotate(obj.model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
			}
			else if (obj.rotateZ)
			{
				obj.model = glm::rotate(obj.model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
			}
			obj.model = glm::scale(obj.model, glm::vec3(obj.tamanhoEscala));

			obj.model = glm::translate(obj.model, glm::vec3(obj.posX, obj.posY, obj.posZ));

			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(obj.model));

			glUniform1f(glGetUniformLocation(shaderID, "ka"), obj.material.ka.r); 
            glUniform1f(glGetUniformLocation(shaderID, "kd"), obj.material.kd.r);
            glUniform1f(glGetUniformLocation(shaderID, "ks"), obj.material.ks.r);

			glBindVertexArray(obj.VAO); // Conectando ao buffer de geometria
			glBindTexture(GL_TEXTURE_2D, obj.texID); //conectando com o buffer de textura que será usado no draw
			glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
		}

		glBindVertexArray(0); // Desconectando o buffer de geometria

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	for (Object& obj : objs) {
		glDeleteVertexArrays(1, &obj.VAO);
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
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
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

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

int loadSimpleOBJ(string filePATH, int &nVertices)
 {
    std::string nomeArquivoMtl;
    float ka, ks, ke;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);
    GLuint texturaId = 0;

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) 
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line)) 
	{
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "mtllib") 
		{
            ssline >> nomeArquivoMtl;
        } 
        if (word == "v") 
		{
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } 
        else if (word == "vt") 
		{
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } 
        else if (word == "vn") 
		{
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } 
        else if (word == "f")
		 {
            while (ssline >> word) 
			{
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);

                if (ti >= 0 && ti < texCoords.size()) {
                    vBuffer.push_back(texCoords[ti].x);
                    vBuffer.push_back(texCoords[ti].y);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
                
                // Adicionando normais (nx, ny, nz)
                if (ni >= 0 && ni < normals.size()) {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }

    arqEntrada.close();

    if (!nomeArquivoMtl.empty()) {
        std::string diretorioObj = filePATH.substr(0, filePATH.find_last_of("/\\") + 1);
        std::string caminhoMTL = diretorioObj + nomeArquivoMtl;
        std::ifstream arqMTL(caminhoMTL.c_str());
        if (arqMTL.is_open()) {
            std::string mtlLine;
            while (std::getline(arqMTL, mtlLine)) {
                std::istringstream ssmtl(mtlLine);
                std::string mtlWord;
                ssmtl >> mtlWord;

                if (mtlWord == "newmtl") {
                    ssmtl >> nomeMaterial;
                    materiais[nomeMaterial] = Material();
                    cout << "Nome material: " << nomeMaterial << endl;
                }

                if (mtlWord == "Ka") {
                    ssmtl >> materiais[nomeMaterial].ka.r >> materiais[nomeMaterial].ka.g >> materiais[nomeMaterial].ka.b;
                }

                if (mtlWord == "Kd") {
                    ssmtl >> materiais[nomeMaterial].kd.r >> materiais[nomeMaterial].kd.g >> materiais[nomeMaterial].kd.b;
                }

                if (mtlWord == "Ks") {
                    ssmtl >> materiais[nomeMaterial].ks.r >> materiais[nomeMaterial].ks.g >> materiais[nomeMaterial].ks.b;
                }

                if (mtlWord == "map_Kd") {
                    ssmtl >> materiais[nomeMaterial].textureFile;
                    cout << "Nome arquivo textura: " << materiais[nomeMaterial].textureFile << endl;
                }
            }
            arqMTL.close();
        }
    }   

    std::cout << "Gerando o buffer de geometria..." << std::endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

	GLsizei stride = 11 * sizeof(GLfloat);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);                    // posição
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat))); // cor
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat))); // texcoord
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(8 * sizeof(GLfloat))); // normal

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	nVertices = vBuffer.size() / 11;  // x, y, z, r, g, b, s, t, nx, ny, nz (valores atualmente armazenados por vértice)
	cout << "nVertices: " << nVertices << endl;
    return VAO;
}