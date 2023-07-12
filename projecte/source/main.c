// Simple citro2d sprite drawing example
// Images borrowed from:
//   https://kenney.nl/assets/space-shooter-redux
#include <citro2d.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define MAX_SPRITES   768
#define SCREEN_WIDTH_UP  400
#define SCREEN_WIDTH_DOWN  320
#define SCREEN_HEIGHT 240
#define COLUMNS 8
#define ROWS 6

// Simple sprite struct
typedef struct
{
	C2D_Sprite spr;
	bool active;
	bool alreadyAttacked;
	int attackSpeed; //velocidad de recarga
	int lastShot; //ultimo segundo en el que disparo, para saber cuando vuelve a atacar
	int type; // 3, 5, 7 para corresponder con los sprites (aunque realmente da igual)
	float rangeAttack; // distancia a la que llega a atacar
	float areaAttack; // si ataca de manera multiple, a que area afecta
	int damage; //damage que hace a los enemigos
	float dx, dy; // velocity
} Sprite;

typedef struct
{
	C2D_Sprite spr;
	bool alive;
	int money; //dinero que suelta al morir
	int type; //tipo de enemigo
	int health; // vida del enemigo
	float totalHealth;
	int damage; //dano que hace a la torre
	float speed; // velocity
} SpriteEnemy;

static C2D_SpriteSheet spriteSheet;
static Sprite sprites[MAX_SPRITES];
static Sprite towers[ROWS][COLUMNS];
static SpriteEnemy enemies[MAX_SPRITES];
static SpriteEnemy enemyLifeBars[MAX_SPRITES];
int table[6][8] = {};
int firstEnemyAlive = 0;
int enemiesGenerated = 0;
int enemiesHordes = 0;
int dinero = 500;
int price = 0;
char buf[8];
char buf2[8];
int towerLife = 2000;
bool playing = true;
bool win = true;
int choosing = -1;
//static size_t numSprites = MAX_SPRITES/2;
int aux_seconds = 0;
int aux_milSeconds = 0;
bool assigned = false;

C2D_TextBuf g_staticBuf;
C2D_Text g_staticText[4];

float getDistance(float x1, float y1, float x2, float y2){
	return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

int mapRow(int y)
{
	if (y < 142)
	{
		if (y < 81)
		{
			return 0;
			//return 20.f;
		}
		else if (y < 111)
		{
			return 1;
			//return 60.f;
		}
		else
		{
			return 2;
			//return 100.f;
		}
	}
	else
	{
		if (y < 171)
		{
			return 3;
			//return 140.f;
		}
		else if (y < 201)
		{
			return 4;
			//return 180.f;
		}
		else
		{
			return 5;
			//return 220.f;
		}
	}
}

//---------------------------------------------------------------------------------
static void initSprites() {
//---------------------------------------------------------------------------------
	size_t numImages = C2D_SpriteSheetCount(spriteSheet);
	srand(time(NULL));

	for (size_t i = 0; i < numImages; i++)
	{
		Sprite* sprite = &sprites[i];

		// Random image, position, rotation and speed
		C2D_SpriteFromSheet(&sprite->spr, spriteSheet, i); // el tercer argument era rand() % numImages
		C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
		C2D_SpriteSetPos(&sprite->spr, rand() % SCREEN_WIDTH_UP, rand() % SCREEN_HEIGHT);
		//C2D_SpriteSetRotation(&sprite->spr, C3D_Angle(rand()/(float)RAND_MAX));
		//sprite->dx = rand()*4.0f/RAND_MAX - 2.0f;
		//sprite->dy = rand()*4.0f/RAND_MAX - 2.0f;
	}
}

//---------------------------------------------------------------------------------
static void moveSprites() {
//---------------------------------------------------------------------------------
	for (size_t i = firstEnemyAlive; i < enemiesGenerated; i++)
	{
		
		SpriteEnemy* sprite = &enemies[i];
		if(!(sprite->alive)) continue;
		SpriteEnemy* lifeBar = &enemyLifeBars[i];
		C2D_SpriteMove(&sprite->spr, sprite->speed, 0.f);
		C2D_SpriteMove(&lifeBar->spr, sprite->speed, 0.f);
		
		if (sprite->health <= 0){
			sprite->alive = false;
			dinero += sprite->money;
			if(i == firstEnemyAlive) {
				firstEnemyAlive++;
			} else {
				SpriteEnemy* firstAlive = &enemies[firstEnemyAlive];
				while(!(firstAlive->alive) && firstEnemyAlive < enemiesGenerated){
					firstEnemyAlive++;
					firstAlive = &enemies[firstEnemyAlive];
				}
			}
			itoa(dinero, buf, 10);
			C2D_TextParse(&g_staticText[1], g_staticBuf, buf);
			C2D_TextOptimize(&g_staticText[1]);
			continue;
		}
		if (sprite->spr.params.pos.x > 335.f){
			towerLife -= sprite->damage;
			if(towerLife <= 0) {
				towerLife = 0;
				win = false;
				playing = false;
			}
			sprite->alive = false;
			if(i == firstEnemyAlive) {
				firstEnemyAlive++;
			} else {
				SpriteEnemy* firstAlive = &enemies[firstEnemyAlive];
				while(!(firstAlive->alive) && firstEnemyAlive < enemiesGenerated){
					firstEnemyAlive++;
					firstAlive = &enemies[firstEnemyAlive];
				}
			}
			itoa(towerLife, buf2, 10);
			C2D_TextParse(&g_staticText[3], g_staticBuf, buf2);
			C2D_TextOptimize(&g_staticText[3]);
		}
	}
}

void fillTower(int i, int j){
	if(table[i][j] == 0){
		table[i][j] = 1;
		Sprite* sprite = &towers[i][j];
		sprite->active = true;
		sprite->type = choosing;
		switch(choosing){
			case 3:
				sprite->damage = 5;
				sprite->rangeAttack = 80;
				sprite->attackSpeed = 60;
				sprite->lastShot = 60;
				break;
			case 5:
				sprite->damage = 1;
				sprite->rangeAttack = 80;
				sprite->attackSpeed = 6;
				sprite->lastShot = 6;
				break;
			case 7:
				sprite->damage = 15;
				sprite->rangeAttack = 120;
				sprite->attackSpeed = 180;
				sprite->lastShot = 180;
				sprite->areaAttack = 35;
				break;
		}
		C2D_SpriteFromSheet(&sprite->spr, spriteSheet, choosing); // el tercer argument era rand() % numImages
		C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
		C2D_SpriteSetPos(&sprite->spr, 60.f + j * 40.f, 20.f + 40.f * i);
		dinero -= price;
		itoa(dinero, buf, 10);
		C2D_TextParse(&g_staticText[1], g_staticBuf, buf);
		C2D_TextOptimize(&g_staticText[1]);
	}
}

//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------
	// Init libs
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	//consoleInit(GFX_BOTTOM, NULL);

	// Create screens
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	// Load graphics
	spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
	if (!spriteSheet) svcBreak(USERBREAK_PANIC);

	// Initialize sprites
	initSprites();

	bool countdown = true;
	bool started = false;
	int fases = 0;

	for(int i=0; i<ROWS; i++) {
      for(int j=0;j<COLUMNS;j++) {
        table[i][j] = 0;
      }
   	}
	//CAMINO ENEMIGOS Y TORRE
	for(int j=0;j<COLUMNS;j++) {
        table[3][j] = -1;
    }

	g_staticBuf = C2D_TextBufNew(100000);
	C2D_TextParse(&g_staticText[0], g_staticBuf, "Dinero: ");
	C2D_TextOptimize(&g_staticText[0]);
	itoa(dinero, buf, 10);
	C2D_TextParse(&g_staticText[1], g_staticBuf, buf);
	C2D_TextOptimize(&g_staticText[1]);
	C2D_TextParse(&g_staticText[2], g_staticBuf, "Vida torre: ");
	C2D_TextOptimize(&g_staticText[2]);
	itoa(towerLife, buf2, 10);
	C2D_TextParse(&g_staticText[3], g_staticBuf, buf2);
	C2D_TextOptimize(&g_staticText[3]);

	Sprite* upperScreen = &sprites[1];
	upperScreen->spr.params.pos.x = SCREEN_WIDTH_UP-(upperScreen->spr.params.pos.w / 2.0f);
	upperScreen->spr.params.pos.y = SCREEN_HEIGHT-(upperScreen->spr.params.pos.h / 2.0f);

	Sprite* lowerScreen = &sprites[0];
	lowerScreen->spr.params.pos.x = SCREEN_WIDTH_DOWN-(lowerScreen->spr.params.pos.w / 2.0f);
	lowerScreen->spr.params.pos.y = SCREEN_HEIGHT-(lowerScreen->spr.params.pos.h / 2.0f);

	Sprite* titleScreen = &sprites[21];
	titleScreen->spr.params.pos.x = 200.f;
	titleScreen->spr.params.pos.y = 120.f;

	Sprite* titleScreenDown = &sprites[22];
	titleScreenDown->spr.params.pos.x = 160.f;
	titleScreenDown->spr.params.pos.y = 120.f;

	Sprite* tower0 = &sprites[2];
	tower0->spr.params.pos.x = 60.f;
	tower0->spr.params.pos.y = 28.f;

	Sprite* tower1 = &sprites[4];
	tower1->spr.params.pos.x = 115.f;
	tower1->spr.params.pos.y = 28.f;


	Sprite* tower2 = &sprites[6];
	tower2->spr.params.pos.x = 171.f;
	tower2->spr.params.pos.y = 28.f;

	//TORRE
	Sprite* tower3 = &sprites[8];
	tower3->spr.params.pos.x = 340.f;
	tower3->spr.params.pos.y = 140.f;
	float towerPosX = 340.f;

	Sprite* number1 = &sprites[9];
	number1->spr.params.pos.x = 160.f;
	number1->spr.params.pos.y = 120.f;

	Sprite* number2 = &sprites[10];
	number2->spr.params.pos.x = 160.f;
	number2->spr.params.pos.y = 120.f;

	Sprite* number3 = &sprites[11];
	number3->spr.params.pos.x = 160.f;
	number3->spr.params.pos.y = 120.f;

	Sprite* gameOver = &sprites[15];
	gameOver->spr.params.pos.x = 160.f;
	gameOver->spr.params.pos.y = 120.f;

	Sprite* victory = &sprites[16];
	victory->spr.params.pos.x = 160.f;
	victory->spr.params.pos.y = 120.f;

	Sprite* startScreen = &sprites[20];
	startScreen->spr.params.pos.x = 160.f;
	startScreen->spr.params.pos.y = 120.f;

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		time_t unixTime = time(NULL);
		struct tm* timeStruct = gmtime((const time_t *)&unixTime);

		int seconds = timeStruct->tm_sec;
		
		/*
		struct timeval te; 
		gettimeofday(&te, NULL); // get current time
		int milliseconds = te.tv_usec; // va de 0000 a 999000
		*/

		// Respond to user input
		u32 kDown = hidKeysDown();
		//u32 kHeld = hidKeysHeld();

		if (kDown & KEY_START){
			break; // break in order to return to hbmenu
		}

		touchPosition touch;
		hidTouchRead(&touch);

		if (touch.py > 8 && touch.py < 47 && playing){
			if(touch.px > 40 && touch.px < 79){
				choosing = 3;
				price = 100;
			} else if(touch.px > 95 && touch.px < 134){
				choosing = 5;
				price = 200;
			} else if(touch.px > 151 && touch.px < 190){
				choosing = 7;
				price = 400;
			}
		} else if(touch.px > 40 && touch.px < 279 && touch.py > 51 && touch.py < 230 && choosing > 0 && dinero >= price && playing && started)
		{
			if (touch.px < 160)
			{
				if (touch.px < 100)
				{
					if (touch.px < 70)
					{
						fillTower(mapRow(touch.py), 0);
						//printf("\x1b[2;0H%c%d", 'A', mapRow(touch.py));
					}
					else
					{
						fillTower(mapRow(touch.py), 1);
						//printf("\x1b[2;0H%c%d", 'B', mapRow(touch.py));
					}
				}
				else
				{
					if (touch.px < 130)
					{
						fillTower(mapRow(touch.py), 2);
						//printf("\x1b[2;0H%c%d", 'C', mapRow(touch.py));
					}
					else
					{
						fillTower(mapRow(touch.py), 3);
						//printf("\x1b[2;0H%c%d", 'D', mapRow(touch.py));
					}
				}
			}
			else
			{
				if (touch.px < 220)
				{
					if (touch.px < 190)
					{
						fillTower(mapRow(touch.py), 4);
						//printf("\x1b[2;0H%c%d", 'E', mapRow(touch.py));
					}
					else
					{
						fillTower(mapRow(touch.py), 5);
						//printf("\x1b[2;0H%c%d", 'F', mapRow(touch.py));
					}
				}
				else
				{
					if (touch.px < 250)
					{
						fillTower(mapRow(touch.py), 6);
						//printf("\x1b[2;0H%c%d", 'G', mapRow(touch.py));
					}
					else
					{
						fillTower(mapRow(touch.py), 7);
						//printf("\x1b[2;0H%c%d", 'H', mapRow(touch.py));
					}
				}
			}
		} else if(!started){
			if (touch.py > 84 && touch.py < 156 && touch.px > 59 && touch.px < 259){
				started = true;
			}
		} else
		{
			if(touch.px != 0 && touch.py != 0){
				choosing = -1;
			}
		}



		if(!countdown && playing){
			switch(fases){
				case 1:
					if(enemiesHordes < 20) {
						if(!assigned){
							aux_seconds = seconds;
							assigned = true;
						} else{
							if ((seconds > aux_seconds && abs(seconds - aux_seconds) >= 2) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 2)){
								assigned = false;
								SpriteEnemy* sprite = &enemies[enemiesGenerated];

								C2D_SpriteFromSheet(&sprite->spr, spriteSheet, 12);
								C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&sprite->spr, 60.f, 140.f);
								sprite->alive = true;
								sprite->speed = 0.5f;
								sprite->damage = 100;
								sprite->money = 20;
								sprite->health = 20;
								sprite->totalHealth = 20;

								
								SpriteEnemy* lifeBar = &enemyLifeBars[enemiesGenerated];
								C2D_SpriteFromSheet(&lifeBar->spr, spriteSheet, 17);
								C2D_SpriteSetCenter(&lifeBar->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&lifeBar->spr, 60.f, 125.f);
								enemiesGenerated++;
								enemiesHordes++;
							}
						}
					} else {
						fases = 5;
						enemiesHordes = 0;
					}
					break;
				case 2:
					if(enemiesHordes < 20) {
						if(!assigned){
							aux_seconds = seconds;
							assigned = true;
						} else{
							if ((seconds > aux_seconds && abs(seconds - aux_seconds) >= 1) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 1)){
								assigned = false;
								SpriteEnemy* sprite = &enemies[enemiesGenerated];

								C2D_SpriteFromSheet(&sprite->spr, spriteSheet, 13); // el tercer argument era rand() % numImages
								C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&sprite->spr, 60.f, 140.f);
								sprite->alive = true;
								sprite->speed = 1.0f;
								sprite->damage = 100;
								sprite->money = 10;
								sprite->health = 18;
								sprite->totalHealth = 18;

								SpriteEnemy* lifeBar = &enemyLifeBars[enemiesGenerated];
								C2D_SpriteFromSheet(&lifeBar->spr, spriteSheet, 18);
								C2D_SpriteSetCenter(&lifeBar->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&lifeBar->spr, 60.f, 125.f);

								enemiesGenerated++;
								enemiesHordes++;
							}
						}
					} else {
						fases = 6;
						enemiesHordes = 0;
					}
					break;
				case 3:
					if(enemiesHordes < 10) {
						if(!assigned){
							aux_seconds = seconds;
							assigned = true;
						} else{
							if ((seconds > aux_seconds && abs(seconds - aux_seconds) >= 3) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 3)){
								assigned = false;
								SpriteEnemy* sprite = &enemies[enemiesGenerated];

								C2D_SpriteFromSheet(&sprite->spr, spriteSheet, 14); // el tercer argument era rand() % numImages
								C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&sprite->spr, 60.f, 140.f);
								sprite->alive = true;
								sprite->speed = 0.15f;
								sprite->damage = 300;
								sprite->money = 100;
								sprite->health = 300;
								sprite->totalHealth = 300;

								SpriteEnemy* lifeBar = &enemyLifeBars[enemiesGenerated];
								C2D_SpriteFromSheet(&lifeBar->spr, spriteSheet, 19);
								C2D_SpriteSetCenter(&lifeBar->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&lifeBar->spr, 60.f, 125.f);

								enemiesGenerated++;
								enemiesHordes++;
							}
						}
					} else {
						fases = 7;
						enemiesHordes = 0;
					}
					break;
				case 4:
					if(enemiesHordes < 30) {
						if(!assigned){
							aux_seconds = seconds;
							assigned = true;
						} else{
							if ((seconds > aux_seconds && abs(seconds - aux_seconds) >= 1) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 1)){
								
								int enemyN = rand() % 3;
								assigned = false;
								SpriteEnemy* sprite = &enemies[enemiesGenerated];
								SpriteEnemy* lifeBar = &enemyLifeBars[enemiesGenerated];
								switch(enemyN){
									case 0:
										C2D_SpriteFromSheet(&sprite->spr, spriteSheet, 12); // el tercer argument era rand() % numImages
										sprite->speed = 0.5f;
										sprite->damage = 100;
										sprite->money = 20;
										sprite->health = 20;
										sprite->totalHealth = 20;

										C2D_SpriteFromSheet(&lifeBar->spr, spriteSheet, 17);
										break;
									case 1:
										C2D_SpriteFromSheet(&sprite->spr, spriteSheet, 13); // el tercer argument era rand() % numImages
										sprite->speed = 1.0f;
										sprite->damage = 100;
										sprite->money = 10;
										sprite->health = 18;
										sprite->totalHealth = 18;

										C2D_SpriteFromSheet(&lifeBar->spr, spriteSheet, 18);
										break;
									case 2:
										C2D_SpriteFromSheet(&sprite->spr, spriteSheet, 14); // el tercer argument era rand() % numImages
										sprite->speed = 0.15f;
										sprite->damage = 300;
										sprite->money = 100;
										sprite->health = 300;
										sprite->totalHealth = 300;

										C2D_SpriteFromSheet(&lifeBar->spr, spriteSheet, 19);
										break;
								}
								C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&sprite->spr, 60.f, 140.f);

								C2D_SpriteSetCenter(&lifeBar->spr, 0.5f, 0.5f);
								C2D_SpriteSetPos(&lifeBar->spr, 60.f, 125.f);
								sprite->alive = true;
								
								enemiesGenerated++;
								enemiesHordes++;
							}
						}
					} else {
						fases = 8;
						enemiesHordes = 0;
					}
					break;
				case 5:
					if(!assigned){
						aux_seconds = seconds;
						assigned = true;
					} else{
						if ((seconds > aux_seconds && abs(seconds - aux_seconds) >= 4) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 4)){
							assigned = false;
							fases = 2;
						}
					}
					break;
				case 6:
					if(!assigned){
						aux_seconds = seconds;
						assigned = true;
					} else{
						if ((seconds > aux_seconds && abs(seconds - aux_seconds) >= 3) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 3)){
							assigned = false;
							fases = 3;
						}
					}
					break;
				case 7:
					if(!assigned){
						aux_seconds = seconds;
						assigned = true;
					} else{
						if ((seconds > aux_seconds && abs(seconds - aux_seconds) >= 13) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 13)){
							assigned = false;
							fases = 4;
						}
					}
					break;
				case 8:
					bool allDead = true;
					for (size_t i = firstEnemyAlive; i < enemiesGenerated; i++){
						SpriteEnemy* sprite = &enemies[i];
						if((sprite->alive)) allDead = false;
					}
					if(allDead){
						playing = false;
						win = true;
					}
					break;
			}
		}

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
		C2D_TargetClear(bottom,  C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
		C2D_SceneBegin(bottom);
		if(started){
		C2D_DrawSprite(&sprites[0].spr);
		C2D_DrawSprite(&sprites[2].spr);
		C2D_DrawSprite(&sprites[4].spr);
		C2D_DrawSprite(&sprites[6].spr);
		C2D_DrawText(&g_staticText[0], C2D_AtBaseline | C2D_WithColor | C2D_AlignRight, 270.0f, 45.0f, 0.5f, 0.5f, 0.75f, C2D_Color32f(1.0f, 0.0f, 0.0f, 1.0f));
		C2D_DrawText(&g_staticText[1], C2D_AtBaseline | C2D_WithColor | C2D_AlignRight, 315.0f, 45.0f, 0.5f, 0.5f, 0.75f, C2D_Color32f(1.0f, 0.0f, 0.0f, 1.0f));
		C2D_DrawText(&g_staticText[2], C2D_AtBaseline | C2D_WithColor | C2D_AlignRight, 270.0f, 30.0f, 0.5f, 0.5f, 0.75f, C2D_Color32f(1.0f, 0.0f, 0.0f, 1.0f));
		C2D_DrawText(&g_staticText[3], C2D_AtBaseline | C2D_WithColor | C2D_AlignRight, 315.0f, 30.0f, 0.5f, 0.5f, 0.75f, C2D_Color32f(1.0f, 0.0f, 0.0f, 1.0f));
		} else {
			C2D_DrawSprite(&sprites[22].spr);
			C2D_DrawSprite(&sprites[20].spr);
		}
		if(countdown && started){
			if(!assigned){
				aux_seconds = seconds;
				assigned = true;
			} else{
				if ((seconds >= aux_seconds && abs(seconds - aux_seconds) >= 0 && abs(seconds - aux_seconds) <= 1) || (seconds <= aux_seconds && ((60 - aux_seconds) + seconds) >= 0 && ((60 - aux_seconds) + seconds) <= 1)){
					C2D_DrawSprite(&sprites[11].spr);
				} else if((seconds > aux_seconds && abs(seconds - aux_seconds) >= 1 && abs(seconds - aux_seconds) <= 2) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 1 && ((60 - aux_seconds) + seconds) <= 2)){
					C2D_DrawSprite(&sprites[10].spr);
				} else if((seconds > aux_seconds && abs(seconds - aux_seconds) >= 2 && abs(seconds - aux_seconds) <= 3) || (seconds < aux_seconds && ((60 - aux_seconds) + seconds) >= 2 && ((60 - aux_seconds) + seconds) <= 3)){
					C2D_DrawSprite(&sprites[9].spr);
				} else {
					countdown = false;
					fases = 1;
					assigned = false;
				}
			}
		}
		if(!playing) {
			if(!win){
				C2D_DrawSprite(&sprites[15].spr);
			} else{
				C2D_DrawSprite(&sprites[16].spr);
			}
			
		}
		C2D_SceneBegin(top);
		if(started){
			C2D_DrawSprite(&sprites[1].spr);
		} else {
			C2D_DrawSprite(&sprites[21].spr);
		}
		
		C2D_DrawSprite(&sprites[8].spr);
		//C2D_DrawSprite(&sprites[3].spr);
		for (size_t i = 0; i < ROWS; i++){
			for (size_t j = 0; j < COLUMNS; j++){
				Sprite* sprite = &towers[i][j];
				if((sprite->active)) {
					switch(sprite->type){
						case 3:
							C2D_DrawSprite(&towers[i][j].spr);
							if((sprite->lastShot % sprite->attackSpeed) == 0){
								float minDistance = 1000.f;
								int closestEnemy = 0; //to the Tower
								for (size_t i = firstEnemyAlive; i < enemiesGenerated; i++){
									SpriteEnemy* enemy = &enemies[i];
									if(!(enemy->alive)) continue;
									if(getDistance(sprite->spr.params.pos.x, sprite->spr.params.pos.y, enemy->spr.params.pos.x, enemy->spr.params.pos.y) <= sprite->rangeAttack){
										if(abs(towerPosX - enemy->spr.params.pos.x) < minDistance){
											minDistance = abs(towerPosX - enemy->spr.params.pos.x);
											closestEnemy = i;
										}
									}
								}
								SpriteEnemy* enemyDamaged = &enemies[closestEnemy];
								enemyDamaged->health -= sprite->damage;
								sprite->lastShot += 1;
							} else{
								sprite->lastShot += 1;
							}
							break;
						case 5:
							C2D_DrawSprite(&towers[i][j].spr);
							if((sprite->lastShot % sprite->attackSpeed) == 0){
								for (size_t i = firstEnemyAlive; i < enemiesGenerated; i++){
									SpriteEnemy* enemy = &enemies[i];
									if(!(enemy->alive)) continue;
									if(getDistance(sprite->spr.params.pos.x, sprite->spr.params.pos.y, enemy->spr.params.pos.x, enemy->spr.params.pos.y) <= sprite->rangeAttack){
										enemy->health -= sprite->damage;
									}
								}
								sprite->lastShot += 1;
							} else{
								sprite->lastShot += 1;
							}
							break;
						case 7:
							C2D_DrawSprite(&towers[i][j].spr);
							if((sprite->lastShot % sprite->attackSpeed) == 0){
								float minDistance = 1000.f;
								int closestEnemy = 0; //to the Tower
								for (size_t i = firstEnemyAlive; i < enemiesGenerated; i++){
									SpriteEnemy* enemy = &enemies[i];
									if(!(enemy->alive)) continue;
									if(getDistance(sprite->spr.params.pos.x, sprite->spr.params.pos.y, enemy->spr.params.pos.x, enemy->spr.params.pos.y) <= sprite->rangeAttack){
										if(abs(towerPosX - enemy->spr.params.pos.x) < minDistance){
											minDistance = abs(towerPosX - enemy->spr.params.pos.x);
											closestEnemy = i;
										}
									}
								}
								SpriteEnemy* enemyDamaged = &enemies[closestEnemy];
								for (size_t i = firstEnemyAlive; i < enemiesGenerated; i++){
									SpriteEnemy* enemy = &enemies[i];
									if(!(enemy->alive)) continue;
									if(getDistance(enemyDamaged->spr.params.pos.x, enemyDamaged->spr.params.pos.y, enemy->spr.params.pos.x, enemy->spr.params.pos.y) <= sprite->areaAttack){
										enemy->health -= sprite->damage;
									}
								}
								sprite->lastShot += 1;
							} else{
								sprite->lastShot += 1;
							}
							break;
					}
				}
			}
		}
		moveSprites();

		for (size_t i = firstEnemyAlive; i < enemiesGenerated; i++){
			SpriteEnemy* sprite = &enemies[i];
			if(!(sprite->alive)) continue;
			SpriteEnemy* lifeBar = &enemyLifeBars[i];
			C2D_SpriteSetScale(&lifeBar->spr, (float)sprite->health/sprite->totalHealth,1.f);
			C2D_DrawSprite(&enemies[i].spr);
			C2D_DrawSprite(&enemyLifeBars[i].spr);
			
		}
		C3D_FrameEnd(0);
	}

	// Delete graphics
	C2D_SpriteSheetFree(spriteSheet);

	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}
