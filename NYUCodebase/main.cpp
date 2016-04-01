#ifdef _WINDOWS
	#include <GL/glew.h>
#endif

#ifdef _WINDOWS
	#define _CRT_SECURE_NO_WARNINGS
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "Matrix.h"
#include "ShaderProgram.h"


using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define TILE_SIZE 0.15f
#define LEVEL_HEIGHT 31
#define LEVEL_WIDTH 128
#define SPRITE_COUNT_X 30
#define SPRITE_COUNT_Y 30

// Level Matrix
Matrix modelMatrix;
Matrix projectionMatrix;
Matrix viewMatrix;

//Sprite Matrix
Matrix modelMatrixSprite;
Matrix projectionMatrixSprite;
Matrix viewMatrixSprite;



ShaderProgram* program;

int mapWidth;
int mapHeight;
unsigned int** levelData;
float lastFrameTicks = 0.0f;


float elapsed;
float ticks;
float counter;

int levelState = 0;

GLuint textureID;
SDL_Window* displayWindow;
const Uint8 *keys = SDL_GetKeyboardState(NULL);

class Entity{

public:
	Entity(){
		velocity = 0;
		x = 0;
		y = 0;
		width = 0;
		height = 0;
	}
	Entity(float v, float X, float Y, float h, float w) {
		velocity = v;
		x = X;
		y = Y;
		height = h;
		width = w;
	}
private:
	float velocity;
	float x;
	float y;
	float height;
	float width;
};

class Tile {

public:
	vector<float> vecCoords;
	vector<float> textCoords;
	int textureID;
	Tile() {

	}
	
	Tile(int textID){
		textureID = textID;
	}

	Tile(int textID, vector<float>& vec, vector<float>& text) : textureID(textID), vecCoords(vec), textCoords(text){

	}
	void Draw(ShaderProgram* program, vector<float>& vec, vector<float>& text)
	{
		program->setModelMatrix(modelMatrix);
		program->setProjectionMatrix(projectionMatrix);
		program->setViewMatrix(viewMatrix);
		glUseProgram(program->programID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vec.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, text.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}
private: 

};

Tile** tileArray;



class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float
		size);
	void Draw(ShaderProgram* program);
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

void SheetSprite::Draw(ShaderProgram* program) {
	program->setModelMatrix(modelMatrix);
	modelMatrix.identity();

	glBindTexture(GL_TEXTURE_2D, textureID);
	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size };
	// draw our arrays
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}



bool readHeader(std::ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height"){
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new unsigned int*[mapHeight];
		tileArray = new Tile*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned int[mapWidth];
			tileArray[i] = new Tile[mapWidth];
		}
		return true;
	}
}

bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned int val = (unsigned int)atoi(tile.c_str());
					//cout << (int)val << endl;
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
						tileArray[y][x] = Tile(val - 1);
					}
					else {
						levelData[y][x] = 0;
						//cout << (int)levelData[y][x] << endl;
					}
				}
			}
		}
	}
	return true;
}

void placeEntity(string type, float x, float y){




	x *= TILE_SIZE;

	y *= TILE_SIZE;

	float u = (float)(19 % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	float v = (float)(19.0f / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
	float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
	float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
	
	float vertexDataSprite[] = {
		-.5f * TILE_SIZE, -.5f * TILE_SIZE,
		.5f * TILE_SIZE, -.5f * TILE_SIZE,
		.5f * TILE_SIZE, .5f * TILE_SIZE,
		-.5f * TILE_SIZE, -.5f * TILE_SIZE,
		.5f * TILE_SIZE, .5f * TILE_SIZE,
		-0.5f * TILE_SIZE, .5f * TILE_SIZE

	};
	/*
	float vertexDataSprite[] = {
		-.5, -.5,
		.5, -.5,
		.5, .5,
		-.5, -.5,
		.5, .5,
		-0.5, .5
	};
	*/
	GLfloat texCoordDataSprite[] = {
			19.0f / 30.0f, 1.0f / 30.0f,
			20.0f / 30.0f, 1.0f / 30.0f,
			20.0f / 30.0f, 0.0f / 30.0f,
			19.0f / 30.0f, 1.0f / 30.0f,
			20.0f / 30.0f, 0.0f / 30.0f,
			19.0f / 30.0f, 0.0f / 30.0f
	};

	program->setModelMatrix(modelMatrixSprite);
	modelMatrixSprite.identity();
	modelMatrixSprite.Translate(counter, 0, 0);
	if (levelState == 0){
		modelMatrixSprite.setPosition(x, y, 0);
		levelState = 1;
	}

	
	
	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexDataSprite);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordDataSprite);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);


	

}

bool readEntityData(std::ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
			
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str());
			float placeY = atoi(yPosition.c_str());
			cout << placeX << " " << placeY << endl;
			Entity player(0.0f, placeX, placeY, 21.0f, 21.0f);
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}



GLuint LoadTexture(const char *image_path) {
	SDL_Surface *surface = IMG_Load(image_path);

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	SDL_FreeSurface(surface);

	return textureID;

}

void render() {

	vector<float> vertexData;
	vector<float> texCoordData;


	
	//counter += 0.05f * elapsed;
		
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			//cout << (int)levelData[y][x] << endl;
			//cout << y << " " << x << endl;
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				//cout << TILE_SIZE * x << " " << TILE_SIZE * y << endl;
				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
	}

	if (keys[SDL_SCANCODE_RIGHT]){
		
		counter += 0.5 * elapsed;
	}
	if (keys[SDL_SCANCODE_LEFT]){

		counter -= 0.5 * elapsed;
	}


	
	program->setModelMatrix(modelMatrix);
	modelMatrix.identity();
	modelMatrix.Translate(-3.55, 2, 0);
	//modelMatrix.Translate(-counter, counter,0);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	viewMatrix.identity();
	viewMatrix.Translate(-counter, 0, 0);
	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0,vertexData.size()/2);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);


	cout << counter << endl;
	

}


int main(int argc, char *argv[])
{

#ifdef _WIN32
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
#endif

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif


	
	SDL_Event event;
	bool done = false;



	glViewport(0, 0, 640, 360);
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program->programID);



	ifstream infile("level11.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				break;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[ObjectsLayer]") {
			readEntityData(infile);
		}
	}


	
	

	GLuint spriteSheetTexture = LoadTexture("spritesheet.png");
	textureID = LoadTexture("spritesheet.png");
	cout << "hi";

	//SheetSprite enemy1 = SheetSprite(spriteSheetTexture, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.2, vertices);

	//SheetSprite cat = SheetSprite(spriteSheetTexture, 100.0f / 692.0f, 100.0f / 692.0f, 100.0f / 692.0f, 80.0f / 692.0f, 0.2);
	
	//Tile world = Tile(spriteSheetTexture, vertexData, texCoordData);
	//world.Draw(&program, vertexData, texCoordData);

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
	


		glColor3f(0.0f, 0.0f, 0.0f);



		program->setModelMatrix(modelMatrixSprite);
		program->setProjectionMatrix(projectionMatrixSprite);
		program->setViewMatrix(viewMatrixSprite);
		
		glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
		program->setModelMatrix(modelMatrix);
		
		//float vertices3[] = { 3.35f, -.5f, 1.2f, -.5f, 1.2f, 1.5f, 3.35f, -.5f, 1.2f, 1.5f, 3.35f, 1.5f };
		
		
		float vertices3[] = {
			-1.65f, 1.05f,
			-1.8f, 1.05f,
			-1.8f, 1.2f,
			-1.65f, 1.05f,
			-1.8f, 1.2f,
			-1.65f, 1.2f

		};

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
		glEnableVertexAttribArray(program->positionAttribute);
		
		float texCoords3[] = {
			19.0f / 30.0f, 1.0f / 30.0f,
			20.0f / 30.0f, 1.0f / 30.0f,
			20.0f / 30.0f, 0.0f / 30.0f,
			19.0f / 30.0f, 1.0f / 30.0f,
			20.0f / 30.0f, 0.0f / 30.0f,
			19.0f / 30.0f, 0.0f / 30.0f

		};
		
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords3);
		glEnableVertexAttribArray(program->texCoordAttribute);


		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
		

		//counter += 0.05;

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		render();
		placeEntity("Player", 7, 11);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
