#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "stm32f7xx_hal.h"
#include "GLCD_Config.h"
#include "Board_GLCD.h"

// Level Files
#include "level_01.h" // int level_01_matrix[18][32]

// Hardware definitions
#define wait HAL_Delay
extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

// Game definitions
#define PI 3.141592654
typedef enum directions {UP, DOWN, LEFT, RIGHT} DirectionType;
typedef enum gameStates {PLAY, WIN, LOSE, RESTART} GameStateType;

typedef int moveScore[18];

typedef struct {
	int gridPos[2];
	DirectionType currentDirection;
	DirectionType requestedDirection;
	moveScore moveScore;
} playerGameObject;

typedef struct {
	int gridPos[2];
	int moveScore[18][32];
} enemyGameObject;


// Bring the bellow code into an external file if you can?
#ifdef __RTX
extern uint32_t os_time;
uint32_t HAL_GetTick(void) {
	return os_time;
}
#endif

// System Clocks Configuration
void SystemClock_Config(void) {	
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();
	/* The voltage scaling allows optimizing the power
	consumption when the device is clocked below the
	maximum system frequency. */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/* Enable HSE Oscillator and activate PLL
	with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	HAL_RCC_OscConfig(&RCC_OscInitStruct);
	/* Select PLL as system clock source and configure
	the HCLK, PCLK1 and PCLK2 clocks dividers */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | 
	RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}


// ADC configuration 
ADC_HandleTypeDef hadc;
static void MX_ADC_Init(void)
{
	// ADC Channel Initalizer Struct
	ADC_ChannelConfTypeDef adcChannel;
	
	// GPIO Initalizer Struct
	GPIO_InitTypeDef gpioInit;
 
	// Enable ADC Clock 3
	__HAL_RCC_ADC3_CLK_ENABLE();
	
	// Enable GPIO Clocks
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();

	// Configure Pin A0
	gpioInit.Pin = GPIO_PIN_0; // Pin A0
	gpioInit.Mode = GPIO_MODE_ANALOG; // Analog Input mode
	gpioInit.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &gpioInit); // GPIO Port A
	
	// Configure Pin A1
	gpioInit.Pin = GPIO_PIN_10; // Pin A1
	gpioInit.Mode = GPIO_MODE_ANALOG; // Analog Input mode
	gpioInit.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOF, &gpioInit); // GPIO Port F
	
	// Enable Inturupts
	HAL_NVIC_SetPriority(ADC_IRQn,0,0);
	HAL_NVIC_EnableIRQ(ADC_IRQn);
	
	// Configure ADC
	hadc.Instance = ADC3;
	hadc.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.ScanConvMode = ENABLE;
	hadc.Init.ContinuousConvMode = ENABLE;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.NbrOfDiscConversion = 0;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.NbrOfConversion = 2;
	hadc.Init.DMAContinuousRequests = ENABLE;
	hadc.Init.EOCSelection = DISABLE;
	
	// Init ADC
	HAL_ADC_Init(&hadc);
	
	// Configure Channel Pin A0
	adcChannel.Channel = ADC_CHANNEL_0;
	adcChannel.Rank = 1;
	adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	adcChannel.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc, &adcChannel);
	
	// Configre Channel Pin A1
	adcChannel.Channel = ADC_CHANNEL_8;
	adcChannel.Rank = 2;
	adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	adcChannel.Offset = 0;
	HAL_ADC_ConfigChannel(&hadc, &adcChannel);
	
}

// DMA Configuration
DMA_HandleTypeDef dmac;
void CongigureDMA() {
	
	// Enable DMA 2 Clock 
	__HAL_RCC_DMA2_CLK_ENABLE();
	__DMA2_CLK_ENABLE();
	
	// Configure DMA Acording to DMA2 Map
	dmac.Instance = DMA2_Stream1;
	dmac.Init.Channel = DMA_CHANNEL_2;
	dmac.Init.Direction = DMA_PERIPH_TO_MEMORY;
	dmac.Init.PeriphInc = DMA_PINC_DISABLE;
	dmac.Init.MemInc = DMA_MINC_ENABLE;
	dmac.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	dmac.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
	dmac.Init.Mode = DMA_CIRCULAR;
	dmac.Init.Priority = DMA_PRIORITY_HIGH;
	dmac.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	dmac.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
	dmac.Init.MemBurst = DMA_MBURST_SINGLE;
	dmac.Init.PeriphBurst = DMA_PBURST_SINGLE;
	
	// Init DMA
	HAL_DMA_Init(&dmac);
	__HAL_LINKDMA(&hadc, DMA_Handle, dmac);
	
	// Set DMA Priority
	HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
	
}

void setup(void) {
	HAL_Init(); // Initialize "Hardware Abstraction Layer"
	SystemClock_Config(); //<- Defined in snipped.c from labs
	GLCD_Initialize();
	MX_ADC_Init();
	//GLCD_SetFont(&GLCD_Font_16x24);
	GLCD_SetForegroundColor(GLCD_COLOR_BLUE);
	GLCD_SetBackgroundColor(GLCD_COLOR_BLACK);
	GLCD_ClearScreen();
}

// Draw rectangle and fill
void Draw_Fill_Rect(int xPos, int yPos, int xWidth, int yWidth) {
	int i;
	for(i=0; i<xWidth; i++) {
		GLCD_DrawHLine(xPos, yPos+i, xWidth);
	}
}

// Draw level from level matrix file
void Draw_Level_Matrix(int* level_matrix) {
	int y; // Y Grid coordinate
	int x; // X Grid coordinate
	int y_painter = 0; // Y Screen position used to draw
	int x_painter = 0; // X Screen position used to draw
	for(y=0; y<18; y++) { // Loop over each grid row
		for(x=0; x<32; x++) { // Loop over each grid column
			if(level_01_matrix[y][x] == 1) { // Check if we should draw a wall at current grid position
				Draw_Fill_Rect(x_painter, y_painter, 15, 15); // Draw wall of size 15x15 px
			}
			x_painter += 15; // Increment x painter position by block size (15px)
		}
		x_painter = 0; // Reset x painter position for start of new row
		y_painter += 15; // Increment y painter position by block size (15px)
	}
}

// Draw player as rectangle
void DrawPlayer(playerGameObject* player) {
	Draw_Fill_Rect(player->gridPos[0]*15, player->gridPos[1]*15,30,30);
}

// Change player direction if possible
void handleRequestedDirection(playerGameObject* player, int level[18][32]) {
	if(player->requestedDirection == LEFT) {
		if(level[player->gridPos[1]][player->gridPos[0]-1] == 0 && level[player->gridPos[1]+1][player->gridPos[0]-1] == 0) {
			player->currentDirection = player->requestedDirection;
		}
	} else if(player->requestedDirection == RIGHT) {
		if(level[player->gridPos[1]][player->gridPos[0]+2] == 0 && level[player->gridPos[1]+1][player->gridPos[0]+2] == 0) {
			player->currentDirection = player->requestedDirection;
		}
	} else if(player->requestedDirection == UP) {
		if(level[player->gridPos[1]-1][player->gridPos[0]] == 0 && level[player->gridPos[1]-1][player->gridPos[0]+1] == 0) {
			player->currentDirection = player->requestedDirection;
		}
	} else if(player->requestedDirection == DOWN) {
		if(level[player->gridPos[1]+2][player->gridPos[0]] == 0 && level[player->gridPos[1]+2][player->gridPos[0]+1] == 0) {
			player->currentDirection = player->requestedDirection;
		}
	}
}

void movePlayer(playerGameObject* player, int level[18][32]) {
	if(player->currentDirection == LEFT) {
		if(level[player->gridPos[1]][player->gridPos[0]-1] == 0 && level[player->gridPos[1]+1][player->gridPos[0]-1] == 0) {
			player->gridPos[0] -= 1;
		}
	} else if(player->currentDirection == RIGHT) {
		if(level[player->gridPos[1]][player->gridPos[0]+2] == 0 && level[player->gridPos[1]+1][player->gridPos[0]+2] == 0) {
			player->gridPos[0] += 1;
		}
	} else if(player->currentDirection == UP) {
		if(level[player->gridPos[1]-1][player->gridPos[0]] == 0 && level[player->gridPos[1]-1][player->gridPos[0]+1] == 0) {
			player->gridPos[1] -= 1;
		}
	} else if(player->currentDirection == DOWN) {
		if(level[player->gridPos[1]+2][player->gridPos[0]] == 0 && level[player->gridPos[1]+2][player->gridPos[0]+1] == 0) {
			player->gridPos[1] += 1;
		}
	}
}


void clearEmptyPaths(int level_matrix[18][32], int power[18][32], playerGameObject* player, playerGameObject* enemy1) {
	int y; // Y Grid coordinate
	int x; // X Grid coordinate
	int y_painter = 0; // Y Screen position used to draw
	int x_painter = 0; // X Screen position used to draw

	
	GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
	for(y=0; y<18; y++) { // Loop over each grid row
		for(x=0; x<32; x++) { // Loop over each grid column
			if(level_matrix[y][x] == 0) {
				if(x==player->gridPos[0] && y==player->gridPos[1] || x==player->gridPos[0]+1 && y==player->gridPos[1] || x==player->gridPos[0] && y==player->gridPos[1]+1 || x==player->gridPos[0]+1 && y==player->gridPos[1]+1) {
					// Don't Draw
				} else if(x==enemy1->gridPos[0] && y==enemy1->gridPos[1] || x==enemy1->gridPos[0]+1 && y==enemy1->gridPos[1] || x==enemy1->gridPos[0] && y==enemy1->gridPos[1]+1 || x==enemy1->gridPos[0]+1 && y==enemy1->gridPos[1]+1) {
					// Don't Draw
				} else {
					Draw_Fill_Rect(x_painter, y_painter, 15,15);
				}
			}
			x_painter += 15; // Increment x painter position by block size (15px)
		}
		x_painter = 0; // Reset x painter position for start of new row
		y_painter += 15; // Increment y painter position by block size (15px)
	}
}


void setupPowerPellets(int level[18][32], int power[18][32]) {
	int i;
	int j;
	for(i=0; i<18; i++){
		for(j=0; j<32; j++) {
			if(level[i][j] == 0 && level[i+1][j] == 0 && level[i][j+1] == 0 && level[i+1][j+1] == 0) {
				power[i][j] = 1;
			} else {
				power[i][j] = 0;
			}
		}
	}
	// Clear Bottom row of pellets (because on some levels there is no bottom wall 
	for(i=0; i<32; i++) {
		power[17][i] = 0;
	}
}

void drawPowerPellets(int power[18][32]) {
	int y; // Y Grid coordinate
	int x; // X Grid coordinate
	int y_painter = 0; // Y Screen position used to draw
	int x_painter = 0; // X Screen position used to draw
	GLCD_SetForegroundColor(GLCD_COLOR_GREEN);
	for(y=0; y<18; y++) { // Loop over each grid row
		for(x=0; x<32; x++) {
			if(power[y][x] == 1) {
				Draw_Fill_Rect(x_painter + 12, y_painter + 12, 5,5);
			}
			x_painter += 15;
		}
		x_painter = 0;
		y_painter += 15;
	}
}

void updatePowerPellets(playerGameObject* player, int power[18][32]) {
	power[player->gridPos[1]][player->gridPos[0]] = 0;
}

/*
bool gameWin(powerPelletsObject* powerPellets) {
	int i;
	int j;
	bool flag = true;
	for(i=0; i<18; i++) {
		for(j=0; j<32; j++) {
			if(powerPellets->powerPellets[i][j] == 1) {
				flag = false;
			}
		}
	}
	return flag;
}
*/

/*
void DrawEnemys(int num, ...) {
	int i;
	va_list valist;
	va_start(valist,num);
	for(i=0; i<num; i++) {
		DrawPlayer(va_arg(valist,int*));
	}
}
*/

void checkGameStateChange(GameStateType* state, int power[18][32], playerGameObject* player, playerGameObject* enemy1) {
	int i;
	int j;
	bool pelletFlag = true;
	for(i=0; i<18; i++) {
		for(j=0; j<32; j++) {
			 if(power[i][j]==1) {
				 pelletFlag = false;
			 }
		}
	}
	if(pelletFlag == true) {
		*state = WIN;
	}
	if(player->gridPos[0] == enemy1->gridPos[0] && player->gridPos[1] == enemy1->gridPos[1]) {
		*state = LOSE;
	}
}

void DrawEnemys(playerGameObject* enemy1) {
	GLCD_SetForegroundColor(GLCD_COLOR_RED);
	DrawPlayer(enemy1);
}

void figureEnemyDirection(playerGameObject* enemy, playerGameObject* player) {
	int r = rand() % 100;
	
	if((enemy->gridPos[0] - player->gridPos[0]) * (enemy->gridPos[0] - player->gridPos[0]) > (enemy->gridPos[1] - player->gridPos[1]) * (enemy->gridPos[1] - player->gridPos[1])) {
		// Prefer horizontal movement
		if(enemy->gridPos[0] < player->gridPos[0]) {
			if(r > 30) {
				enemy->requestedDirection = RIGHT;
			} else {
				enemy->requestedDirection = LEFT;
			}
		} else {
			if(r > 30) {
				enemy->requestedDirection = LEFT;
			} else {
				enemy->requestedDirection = RIGHT;
			}
		}
	} else {
		// Prefter vertial movement
		if(enemy->gridPos[1] < player->gridPos[1]) {
			if(r > 20) {
				enemy->requestedDirection = DOWN;
			} else {
				enemy->requestedDirection = UP;
			}
		} else {
			if(r > 20) {
				enemy->requestedDirection = UP;
			} else {
				enemy->requestedDirection = DOWN;
			}
		}
	}
}

uint32_t ADC_VALUES[2];
int powerPellets[18][32];
int main(void) {
	
	// Game variables
	GameStateType gameState;
	
	// Player variables
	playerGameObject player;

	// Enemy variabels
	playerGameObject enemy01;

	// Setup board
	setup();
	GLCD_SetFont(&GLCD_Font_16x24);
	
	// Player starting position
	player.gridPos[0] = 1;
	player.gridPos[1] = 1;
	
	// Enemy starting position
	enemy01.gridPos[0] = 28;
	enemy01.gridPos[1] = 16;
	
	// Start DMA for direct pipe from ADC
	CongigureDMA();
	HAL_ADC_Start_DMA(&hadc, ADC_VALUES, 2);
	
	// Draw level walls once
	GLCD_SetForegroundColor(GLCD_COLOR_BLUE);
	Draw_Level_Matrix(*level_01_matrix);
	
	// Init power pellets
	setupPowerPellets(level_01_matrix, powerPellets);
	
	// Start game
	gameState = PLAY;

	// Game Loop
	while(1) {
		while(gameState==PLAY) {
			
			// GPIO input to request change in direction
			if(ADC_VALUES[0] > 3000) {
				player.requestedDirection = LEFT;
			}
			if(ADC_VALUES[0] < 1000) {
				player.requestedDirection = RIGHT;
			}
			if(ADC_VALUES[1] > 3000) {
				player.requestedDirection = UP;
			}
			if(ADC_VALUES[1] < 1000) {
				player.requestedDirection = DOWN;
			}
			
			// Figure out what direction the enemy's must go
			figureEnemyDirection(&enemy01, &player);
			
			// Handle player movement
			handleRequestedDirection(&player, level_01_matrix);
			movePlayer(&player, level_01_matrix);
			
			
			handleRequestedDirection(&enemy01, level_01_matrix);
			movePlayer(&enemy01, level_01_matrix);
			
			wait(100);
			
			
			
			// Draw player
			GLCD_SetForegroundColor(GLCD_COLOR_YELLOW);
			DrawPlayer(&player);
			
			// Draw enemy
			DrawEnemys(&enemy01);

			// Check if pellets eaten
			updatePowerPellets(&player, powerPellets);
			
			// Reset empty paths to black
			clearEmptyPaths(level_01_matrix, powerPellets, &player, &enemy01);
			
			// Re-draw power pellets
			drawPowerPellets(powerPellets);
			
			// Check if the game is won or lost
			checkGameStateChange(&gameState, powerPellets, &player, &enemy01);
			
		}
		
		while(gameState==WIN) {
			GLCD_DrawString(0,0,"WINNNNERRR");
		}
		while(gameState==LOSE) {
			GLCD_DrawString(0,0,"You Lose:");
			GLCD_DrawString(0,24,"Move the joystick to play again.");
			// GPIO input to request change in direction
			if(ADC_VALUES[0] > 3000) {
				gameState = RESTART;
			}
			if(ADC_VALUES[0] < 1000) {
				gameState = RESTART;
			}
			if(ADC_VALUES[1] > 3000) {
				gameState = RESTART;
			}
			if(ADC_VALUES[1] < 1000) {
				gameState = RESTART;
			}
		}
		
		while(gameState==RESTART) {
			// Player starting position
	player.gridPos[0] = 1;
	player.gridPos[1] = 1;
	
	// Enemy starting position
	enemy01.gridPos[0] = 28;
	enemy01.gridPos[1] = 16;
			GLCD_SetForegroundColor(GLCD_COLOR_BLUE);
			Draw_Level_Matrix(*level_01_matrix);
	
	// Init power pellets
	setupPowerPellets(level_01_matrix, powerPellets);
	
	// Start game
	gameState = PLAY;
		}
	}
}
