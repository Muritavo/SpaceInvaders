#include <iostream>
#include <cmath>

using namespace std;

#include <windows.h>
#include <GL/glut.h>
#include <math.h>
#include <fstream>
#include <vector>
#include <time.h>

const byte
	KEY_UP = 0,
	KEY_DOWN = 1,
	KEY_LEFT = 2,
	KEY_RIGHT = 3,
	KEY_SHIFT = 4;
const int
	CIRCLE_EDGES = 6,				   //Number of edges for the circle drawn for the projectile
	BEZIER_SECTIONS = 10,			   //Número de seções da curva para estimar a distância percorrida
	MAX_PROJECTILES = 100,			   //Número maximo de projéteis
	WIDTH = 500,					   //Largura do espaço
	HEIGHT = 500,					   //Altura do espaço
	SHIP_SPEED = 40,				   //Velocidade da nave/s
	SHIP_ROTATION = 180,			   //Graus de rotação/s
	PROJECTILE_SPEED = SHIP_SPEED * 2, //Velocidade da nave/s
	INTERVAL_BETWEEN_ENEMY_SHOTS = 5;  //Intervalo em segundos entre os tiros dos inimigos

bool activeKeys[5] = {false};

struct Projectile
{
	GLfloat position[2];
	GLfloat directionVector[2];
	int size;
};

struct Ship
{
	int lines;
	int columns;
	vector<int> colors;
	float offsetX;
	float offsetY;
};

struct ShipInstance
{
	Ship model;
	GLfloat position[2];
	GLfloat rotation;
	int lives;
	int lastProjectileIndex;
	Projectile projectile[MAX_PROJECTILES];
	Projectile *projectilePointer[MAX_PROJECTILES];
	GLfloat from[2];
	GLfloat to[2];
	GLfloat intermediate[2];
	GLfloat timeToTravel;
	GLfloat timeTraveled;
};

vector<float> colors;
vector<Ship> models;
Ship playerShip = {
	.lines = 5,
	.columns = 3,
	.colors = {
		1, 2, 1,
		2, 1, 2,
		2, 2, 2,
		2, 2, 2,
		2, 1, 2},
	.offsetX = 1.5,
	.offsetY = 2.5};
vector<ShipInstance> instances;

int lastClock = 0;
double elapsedSeconds;

void updateTick()
{
	int currentClock = glutGet(GLUT_ELAPSED_TIME);
	elapsedSeconds = (currentClock - lastClock) / 1000.0f;
	lastClock = currentClock;
	Sleep(1000.0f / 60.0f);
}

void getRandomPoint(float *points)
{
	*(points + 0) = rand() % WIDTH;
	*(points + 1) = rand() % HEIGHT;
}

/**
 * Multiplica matriz por um valor
 */
void multiplyMatrix(float denominator, float *matrix, int matrixSize)
{
	for (int i = 0; i < matrixSize; i++)
		*(matrix + i) = *(matrix + i) * denominator;
}

/**
 * Soma matrizes
 */
void sumMatrix(float *matrix1, float *matrix2, int matrixSize)
{
	for (int mi = 0; mi < matrixSize; mi++)
		*(matrix1 + mi) = *(matrix1 + mi) + *(matrix2 + mi);
}

/**
 * Calcula a bezier
 */
void bezier(float t, float *p0, float *p1, float *p2, int pointsSize)
{
	multiplyMatrix((1.0f - t) * (1.0f - t), p0, pointsSize);
	multiplyMatrix(2.0f * (1.0f - t) * t, p1, pointsSize);
	multiplyMatrix(t * t, p2, pointsSize);

	sumMatrix(p0, p1, pointsSize);
	sumMatrix(p0, p2, pointsSize);
}

/**
 * Normalizador de valores (Utilizado na normalização das cores)
 */
float normalize(float value, float max, float min)
{
	return (value - min) / max - min;
}

void loadGameModel()
{
	ifstream file("D:/Projetos/GLUTProject/src/game_model.txt");
	string line;
	if (!file.is_open())
	{
		exit(0);
	}

	while (!file.eof())
	{
		int index;
		getline(file, line);
		if (line.find("#CORES") != string::npos)
		{
			getline(file, line);
			int numCores = stoi(line);
			printf("Preparando %d cores\n", numCores);
			index = 0;
			colors.resize(numCores * 4);
			getline(file, line);
			do
			{
				printf("Nova cor\n");
				int indexOfLine = 0;
				indexOfLine = line.find(" ", indexOfLine) + 1;
				colors[index] = normalize(stoi(line.substr(indexOfLine, line.find(" ", indexOfLine) - indexOfLine)), 255, 0);
				indexOfLine = line.find(" ", indexOfLine) + 1;
				colors[index + 1] = normalize(stof(line.substr(indexOfLine, line.find(" ", indexOfLine) - indexOfLine)), 255, 0);
				indexOfLine = line.find(" ", indexOfLine) + 1;
				colors[index + 2] = normalize(stoi(line.substr(indexOfLine, line.find(" ", indexOfLine) - indexOfLine)), 255, 0);
				indexOfLine = line.find(" ", indexOfLine) + 1;
				colors[index + 3] = index == 0 ? 0.0f : 1.0f; //A primeira cor sempre é transparente

				printf("Generated color %f, %f, %f, %f\n", colors[index], colors[index + 1], colors[index + 2], colors[index + 3]);
				index += 4;
				getline(file, line);
			} while (!line.empty());
		}
		if (line.find("#OBJETO") != string::npos)
		{
			index = 0;
			do
			{
				//Adiciona mais um modelo de nave
				models.resize(models.size() + 1);
				//Obtem as dimensões da nave
				getline(file, line);
				//Atualiza os atributos da nave
				models[index].lines = stoi(line.substr(0, line.find(" ", 0)));
				models[index].offsetY = models[index].lines / 2;
				models[index].columns = stoi(line.substr(line.find(" ")));
				models[index].offsetX = models[index].columns / 2;
				models[index].colors.resize(models[index].lines * models[index].columns);
				//Variavel auxiliar para separas as cores da linha
				int lastColorIndex = 0;
				//Itera por todas as linhas
				for (int lineIndex = 0; lineIndex < models[index].lines; lineIndex++)
				{
					getline(file, line);
					lastColorIndex = 0;
					for (int columnIndex = 0; columnIndex < models[index].columns; columnIndex++)
					{
						int nextColorIndex = line.find(" ", lastColorIndex) + 1;
						int colorsIndex = lineIndex * models[index].columns + columnIndex;
						models[index].colors[colorsIndex] = stoi(line.substr(lastColorIndex, nextColorIndex - lastColorIndex));
						lastColorIndex = nextColorIndex;
					}
				}
				index++;
			} while (file.peek() != EOF);
		}
	}
}

// **********************************************************************
//  void init(void)
//		Inicializa os parâmetros globais de OpenGL
//
// **********************************************************************
void init(void)
{
	//Adiciona um seed
	srand(time(NULL));
	//Carrega o modelo do jogo
	loadGameModel();

	//Inicializa as instancias
	instances.resize(models.size() + 1);
	//Inicializa o personagem (Índice 0 sempre pertence ao personagem principal)
	instances[0].model = playerShip;
	instances[0].position[0] = WIDTH / 2;
	instances[0].position[1] = HEIGHT / 2;
	instances[0].rotation = 0;
	instances[0].lives = 3;

	//Inicializa as naves inimigas
	for (int mi = 1; mi <= models.size(); mi++)
	{
		instances[mi].model = models[mi - 1];
		instances[mi].position[0] = rand() % WIDTH;
		instances[mi].position[1] = rand() % HEIGHT;
		instances[mi].to[0] = instances[mi].position[0];
		instances[mi].to[1] = instances[mi].position[1];
		instances[mi].lives = 1;
	}

	glDisable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Fundo de tela preto
}

void drawProjectile(Projectile p)
{
	glPushMatrix();
	glTranslatef(p.position[0], p.position[1], 0);
	glTranslatef(-p.size / 2, -p.size / 2, 0);
	glBegin(GL_POLYGON);
	glColor4f(
		1,
		1,
		1,
		1);
	for (int a = 0; a < 360; a += 360 / CIRCLE_EDGES)
		glVertex2f(cos((a * M_PI / 180.0f) * p.size), sin((a * M_PI / 180.0f) * p.size));
	glEnd();
	glPopMatrix();
}

void drawShip(ShipInstance s)
{
	glPushMatrix();
	glTranslatef(s.position[0], s.position[1], 0);
	glRotatef(s.rotation, 0, 0, 1);
	glTranslatef(-s.model.offsetX, -s.model.offsetY, 0);
	glBegin(GL_QUADS);
	for (int li = 0; li < s.model.lines; li++)
	{
		for (int ci = 0; ci < s.model.columns; ci++)
		{
			int colorIndex = ((s.model.colors[(s.model.lines - 1 - li) * s.model.columns + ci] - 1) * 4);
			glColor4f(
				colors[colorIndex],
				colors[colorIndex + 1],
				colors[colorIndex + 2],
				colors[colorIndex + 3]);
			glVertex2f(ci, li);
			glVertex2f(ci + 1, li);
			glVertex2f(ci + 1, li + 1);
			glVertex2f(ci, li + 1);
		}
	}
	glEnd();
	glPopMatrix();

	for (int pi = 0; pi < MAX_PROJECTILES; pi++)
	{
		if (s.projectilePointer[pi] != nullptr)
		{
			drawProjectile(*s.projectilePointer[pi]);
		}
	}
}

void updateProjectilesPosition()
{
	for (int ii = 0; ii < instances.size(); ii++)
	{
		for (int pi = 0; pi < MAX_PROJECTILES; pi++)
		{
			if (instances[ii].projectilePointer[pi] != nullptr)
			{
				Projectile *p = instances[ii].projectilePointer[pi];
				p->position[0] += (p->directionVector[0] * elapsedSeconds);
				p->position[1] += (p->directionVector[1] * elapsedSeconds);
				if ((p->position[0] > WIDTH || p->position[0] < 0) || (p->position[1] > HEIGHT || p->position[1] < 0))
				{
					instances[ii].projectilePointer[pi] = nullptr;
				}
			}
		}
	}
}

void updateEnemiesPosition()
{
	for (int ii = 1; ii < instances.size(); ii++)
	{
		ShipInstance *s = &instances[ii];
		s->rotation = atan2(instances[0].position[1] - s->position[1], instances[0].position[0] - s->position[0]) * 180 / M_PI - 90;
		//Gera um novo destino quando a nave inimiga chegar ao destino
		if (s->position[0] == s->to[0] && s->position[1] == s->to[1])
		{
			s->from[0] = s->position[0];
			s->from[1] = s->position[1];
			getRandomPoint(s->intermediate);
			getRandomPoint(s->to);

			//Estima a distância a ser percorrida
			float distance = 0;
			GLfloat prevPoint[] = {*(s->position + 0), *(s->position + 1)};
			for (int si = 0; si < BEZIER_SECTIONS; si++)
			{
				GLfloat to[] = {*(s->to + 0), *(s->to + 1)};
				GLfloat intermediate[] = {*(s->intermediate + 0), *(s->intermediate + 1)};
				GLfloat nextPoint[] = {*(s->from + 0), *(s->from + 1)};
				bezier(normalize(si, BEZIER_SECTIONS - 1, 0), nextPoint, intermediate, to, 2);
				distance += sqrt(pow(nextPoint[0] - prevPoint[0], 2) + pow(nextPoint[1] - prevPoint[1], 2));
				prevPoint[0] = nextPoint[0];
				prevPoint[1] = nextPoint[1];
			}
			s->timeToTravel = distance / SHIP_SPEED;
			s->timeTraveled = 0;
		}
		else
		{
			s->timeTraveled += elapsedSeconds;
			GLfloat to[] = {*(s->to + 0), *(s->to + 1)};
			GLfloat intermediate[] = {*(s->intermediate + 0), *(s->intermediate + 1)};
			GLfloat nextPoint[] = {*(s->from + 0), *(s->from + 1)};
			if (s->timeTraveled >= s->timeToTravel)
			{
				s->position[0] = s->to[0];
				s->position[1] = s->to[1];
			}
			else
			{
				bezier(normalize(s->timeTraveled, s->timeToTravel, 0), nextPoint, intermediate, to, 2);
				s->position[0] = nextPoint[0];
				s->position[1] = nextPoint[1];
			}
		}
	}
}

void shotProjectile(ShipInstance *s)
{
	s->projectilePointer[s->lastProjectileIndex] = &s->projectile[s->lastProjectileIndex];
	Projectile *pointer = s->projectilePointer[s->lastProjectileIndex];
	pointer->directionVector[0] = PROJECTILE_SPEED * -sin(s->rotation * 3.14f / 180.0f);
	pointer->directionVector[1] = PROJECTILE_SPEED * cos(s->rotation * 3.14f / 180.0f);
	pointer->position[0] = s->position[0] + (s->model.columns / 2);
	pointer->position[1] = s->position[1] + (s->model.lines / 2);
	pointer->size = s->model.columns / 2;
	s->lastProjectileIndex++;
	if (s->lastProjectileIndex == MAX_PROJECTILES)
		s->lastProjectileIndex = 0;
}

void updatePlayerState()
{
	if (activeKeys[KEY_UP] || activeKeys[KEY_DOWN])
	{
		int direct = (activeKeys[KEY_UP]) ? 1.0f : -1.0f;
		int speed = activeKeys[KEY_SHIFT] ? SHIP_SPEED * 1.5f : SHIP_SPEED;
		GLfloat newPosition[2];
		newPosition[0] = instances[0].position[0] + (speed * elapsedSeconds) * -sin(instances[0].rotation * 3.14f / 180.0f) * direct;
		newPosition[1] = instances[0].position[1] + (speed * elapsedSeconds) * cos(instances[0].rotation * 3.14f / 180.0f) * direct;
		if (newPosition[0] > 0 && newPosition[0] < WIDTH)
			instances[0].position[0] = newPosition[0];
		else
			instances[0].position[0] = newPosition[0] > 0 ? WIDTH : 0;

		if (newPosition[1] > 0 && newPosition[1] < HEIGHT)
			instances[0].position[1] = newPosition[1];
		else
			instances[0].position[1] = newPosition[1] > 0 ? HEIGHT : 0;
	}
	if (activeKeys[KEY_LEFT] || activeKeys[KEY_RIGHT])
	{
		int direct = (activeKeys[KEY_LEFT]) ? 1.0f : -1.0f;
		instances[0].rotation += SHIP_ROTATION * elapsedSeconds * direct;
	}
}

void updateEnemiesState()
{
	for (int ii = 1; ii < instances.size(); ii++)
	{
		if (rand() % 200 == 1)
			shotProjectile(&instances[ii]);
	}
}

void keyboard(unsigned char key, bool toogleActive)
{
	activeKeys[KEY_SHIFT] = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? true : false;
	switch (key)
	{
	case 27:	 // Termina o programa qdo
		exit(0); // a tecla ESC for pressionada
		break;
	case GLUT_KEY_UP:
		activeKeys[KEY_UP] = toogleActive;
		break;
	case GLUT_KEY_DOWN:
		activeKeys[KEY_DOWN] = toogleActive;
		break;
	case GLUT_KEY_RIGHT:
		activeKeys[KEY_RIGHT] = toogleActive;
		break;
	case GLUT_KEY_LEFT:
		activeKeys[KEY_LEFT] = toogleActive;
		break;
	default:
		break;
	}
}
void keyboardDown(unsigned char key, int x, int y)
{
	if (key == ' ')
		shotProjectile(&instances[0]);
	keyboard(key, true);
}
void keyboardUp(unsigned char key, int x, int y)
{
	keyboard(key, false);
}
void specialDown(int key, int x, int y)
{
	keyboard(key, true);
}
void specialUp(int key, int x, int y)
{
	keyboard(key, false);
}

// **********************************************************************
//  void display( void )
//
//
// **********************************************************************
void display(void)
{
	updateProjectilesPosition();
	updateEnemiesPosition();
	updateEnemiesState();
	updatePlayerState();
	glClear(GL_COLOR_BUFFER_BIT);
	//Define os limites
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, WIDTH, 0, HEIGHT, 0, HEIGHT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	for (int ii = 0; ii < instances.size(); ii++)
	{
		drawShip(instances[ii]);
	}

	glDisable(GL_BLEND);
	glutSwapBuffers();
	updateTick();
}

// **********************************************************************
//  void main ( int argc, char** argv )
//
//
// **********************************************************************
int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB); // | GLUT_STEREO);// | GLUT_DOUBLE | GLUT_RGBA );

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(800, 800);
	glutCreateWindow("Space Fighters");

	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboardDown);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialFunc(specialDown);
	glutSpecialUpFunc(specialUp);
	glutIdleFunc(display);
	glutMainLoop();
	return 0;
}
