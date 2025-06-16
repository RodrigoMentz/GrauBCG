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
void updateCameraPos(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// Protótipos das funções
int setupShader();
int setupSkyboxShader();
GLuint loadTexture(string filePath, int &width, int &height);
int loadSimpleOBJ(string filePATH, int &nVertices);
GLuint loadCubemap(vector<std::string> faces);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

const GLchar* skyboxVertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view; // Apenas rotação da câmera

out vec3 TexCoords;

void main()
{
    vec4 pos = projection * view * vec4(position, 1.0);
    gl_Position = pos.xyww;
    TexCoords = position;
})";

const GLchar* skyboxFragmentShaderSource = R"(
#version 400
in vec3 TexCoords;

uniform samplerCube skybox;

out vec4 color;

void main()
{
    color = texture(skybox, TexCoords);
})";

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texc;
layout (location = 3) in vec3 normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;
void main()
{
   	gl_Position = projection * view * model * vec4(position.x, position.y, position.z, 1.0);
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

float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

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
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;
//para calcular o quanto que o mouse deslocou
bool firstMouse = true;
float rotacaoYaw = -90.0, rotaocaoPitch = 0.0; //rotação em x e y
float deltaTime = 0.0f; // Tempo entre o frame atual e o anterior
float lastFrame = 0.0f; // Tempo do último frame
float fov = 45.0f;
int indiceObjetoSelecionado = 0;

Object objs[2];

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Prova GB - Rodrigo Korte Mentz", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(window, scroll_callback);
    updateCameraPos(window);

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

    GLuint skyboxShaderID = setupSkyboxShader();

    // Configuração do Skybox VAO/VBO
    GLuint skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    vector<std::string> faces
    {
        "../assets/skybox/posx.jpg",  // +X (direita)
        "../assets/skybox/negx.jpg",  // -X (esquerda)
        "../assets/skybox/posy.jpg",  // +Y (topo)
        "../assets/skybox/negy.jpg", // -Y (fundo)
        "../assets/skybox/posz.jpg",  // +Z (frente)
        "../assets/skybox/negz.jpg"   // -Z (trás)
    };
    GLuint cubemapTexture = loadCubemap(faces);


	objs[0].VAO = loadSimpleOBJ("../assets/Modelos3D/Skeletal_Stego.obj", objs[0].nVertices);
	objs[0].material = materiais["MM_Dino"];
	objs[0].texID = loadTexture("../assets/Modelos3D/" + objs[0].material.textureFile, objs[0].TextureimgWidth,objs[0].TextureimgHeight);

    float q = 10.0;
    vec3 lightPos = vec3(0.6, 1.2, -0.5);
    glm::mat4 view;
	mat4 model = mat4(1); // matriz identidade

    view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),// Posição (ponto)
    glm::vec3(0.0f, 0.0f, 0.0f),// Target (ponto, não vetor)  dir = target - pos
    glm::vec3(0.0f, 1.0f, 0.0f)); // Up (vetor)

    glm::mat4 projection;
    projection = glm::perspective(fov, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
    
	glUseProgram(shaderID);
    
	// Enviar a informação de qual variável armazenará o buffer da textura
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);
	glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x,lightPos.y,lightPos.z);
    
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    // Passa seu conteúdo para o shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // No caso da matriz de projeção, se não mudar não precisa passar a
    // cada iteração
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Obtenha os locais dos uniformes para o shader do skybox
    GLint skyboxViewLoc = glGetUniformLocation(skyboxShaderID, "view");
    GLint skyboxProjLoc = glGetUniformLocation(skyboxShaderID, "projection");

    // Defina o unit de textura para o cubemap
    glUseProgram(skyboxShaderID);
    glUniform1i(glGetUniformLocation(skyboxShaderID, "skybox"), 0); // Skybox usará a unidade de textura 0


    cameraPos.x = -50.8586;
    cameraPos.y = 19.1345;
    cameraPos.z = 30.2106;

    cameraFront.x = 0.823627;
    cameraFront.y = -0.12793;
    cameraFront.z = -0.567121;

    cameraUp.x = 0.10637;
    cameraUp.y = 0.998971;
    cameraUp.z = -0.0710744;
    rotacaoYaw = -37.45;
    rotaocaoPitch = -0.85;

	//Ativando o primeiro buffer de textura da OpenGL
	glActiveTexture(GL_TEXTURE0);
	
	glEnable(GL_DEPTH_TEST);


	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
		glfwPollEvents();
        updateCameraPos(window);

        glm::vec3 front;
        front.x = cos(glm::radians(rotaocaoPitch)) * cos(glm::radians(rotacaoYaw));
        front.y = sin(glm::radians(rotaocaoPitch));
        front.z = cos(glm::radians(rotaocaoPitch)) * sin(glm::radians(rotacaoYaw));
        cameraFront = glm::normalize(front);

        glm::vec3 right = glm::normalize(glm::cross(cameraFront,
        glm::vec3(0.0,1.0,0.0)));
        cameraUp = glm::normalize(glm::cross(right, cameraFront));

        glm::mat4 view;
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        projection = glm::perspective(fov, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(glGetUniformLocation(shaderID, "camPos"), cameraPos.x, cameraPos.y, cameraPos.z);

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // ----------------------------------------------------
        // 1. Desenhar o Skybox
        // ----------------------------------------------------
        glDepthFunc(GL_LEQUAL); // Mude a função de profundidade: passa se Z < ou = ao Z atual
        glUseProgram(skyboxShaderID);

        // A matriz 'view' do skybox deve remover a translação da câmera
        // Isso faz com que o skybox gire com a câmera, mas permaneça no "infinito"
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // Remove a parte da translação (última coluna/linha)
        glUniformMatrix4fv(skyboxViewLoc, 1, GL_FALSE, glm::value_ptr(skyboxView));
        glUniformMatrix4fv(skyboxProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0); // Ativa a unidade de textura 0
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture); // Liga a textura cubemap
        glDrawArrays(GL_TRIANGLES, 0, 36); // O cubo tem 36 vértices (6 faces * 2 triângulos/face * 3 vértices/triângulo)
        glBindVertexArray(0); // Desliga o VAO do skybox
        glDepthFunc(GL_LESS); // Volte para a função de profundidade padrão
        // ----------------------------------------------------

        glUseProgram(shaderID); // Ativa o shader dos seus objetos
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view)); // Use a view normal aqui
        glUniform3f(glGetUniformLocation(shaderID, "camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    
        glUniform1f(glGetUniformLocation(shaderID, "q"), q);
        glUniform3f(glGetUniformLocation(shaderID, "lightPos"), lightPos.x,lightPos.y,lightPos.z);

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

        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            cout << "camera x: " << cameraPos.x << endl;
            cout << "camera y: " << cameraPos.y << endl;
            cout << "camera z: " << cameraPos.z << endl;
            cout << "camerafront x: " << cameraFront.x << endl;
            cout << "camerafront y: " << cameraFront.y << endl;
            cout << "camerafront z: " << cameraFront.z << endl;
            cout << "cameraUp x: " << cameraUp.x << endl;
            cout << "cameraUp y: " << cameraUp.y << endl;
            cout << "cameraUp z: " << cameraUp.z << endl;
            cout << "rotacaoYaw: " << rotacaoYaw << endl;
            cout << "rotaocaoPitch: " << rotaocaoPitch << endl;
        }

        if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        indiceObjetoSelecionado = 0;
        cout << "Objeto 1 selecionado" << endl;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        if (sizeof(objs) / sizeof(objs[0]) > 1) {
            indiceObjetoSelecionado = 1;
            cout << "Objeto 2 selecionado" << endl;
        } else {
            cout << "Objeto 2 não existe" << endl;
        }
    }

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].rotateX = true;
        objs[indiceObjetoSelecionado].rotateY = false;
        objs[indiceObjetoSelecionado].rotateZ = false;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].rotateX = false;
        objs[indiceObjetoSelecionado].rotateY = true;
        objs[indiceObjetoSelecionado].rotateZ = false;
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].rotateX = false;
        objs[indiceObjetoSelecionado].rotateY = false;
        objs[indiceObjetoSelecionado].rotateZ = true;
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].posY += 0.1f;
    }

    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].posY -= 0.1f;
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].posX -= 0.1f;
    }

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].posX += 0.1f;
    }

    if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS) // Tecla + do teclado numérico
    {
        objs[indiceObjetoSelecionado].posZ += 0.1f;
    }
    
    if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS) // Tecla - do teclado numérico
    {
        objs[indiceObjetoSelecionado].posZ -= 0.1f;
    }

    if (key == GLFW_KEY_U && action == GLFW_PRESS)
    {
        objs[indiceObjetoSelecionado].tamanhoEscala += 0.1f;
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        if (objs[indiceObjetoSelecionado].tamanhoEscala < 0.1f)
        {
            objs[indiceObjetoSelecionado].tamanhoEscala = 0.1f;
        } else {
            objs[indiceObjetoSelecionado].tamanhoEscala -= 0.1f;
        }
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos; // Invertido porque as coordenadas Y da tela geralmente são de cima para baixo
    lastX = xpos;
    lastY = ypos;

    GLfloat sensitivity = 0.05f; // Ajuste a sensibilidade do mouse aqui
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    rotacaoYaw += xoffset;
    rotaocaoPitch += yoffset;

    // Restrições para o pitch para evitar "virar" a câmera de cabeça para baixo
    if (rotaocaoPitch > 89.0f)
    {
        rotaocaoPitch = 89.0f;
    }

    if (rotaocaoPitch < -89.0f)
    {
        rotaocaoPitch = -89.0f;
    }
}

void updateCameraPos(GLFWwindow *window)
{
    float cameraSpeed = 0.05f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        cameraPos += cameraSpeed * cameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        cameraPos -= cameraSpeed * cameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(fov >= 1.0f && fov <= 45.0f)
    {
        fov -= yoffset * 0.1f;
    }
    if(fov <= 1.0f)
    {
        fov = 1.0f;
    }
    if(fov >= 45.0f)
    {
        fov = 45.0f;
    }
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

int setupSkyboxShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &skyboxVertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &skyboxFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

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

GLuint loadCubemap(vector<std::string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (GLuint i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format;
            if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;
            else format = GL_RGB; // Default to RGB

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); // GL_TEXTURE_WRAP_R é para texturas 3D/cubemaps

    return textureID;
}