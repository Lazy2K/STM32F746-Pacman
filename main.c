#include <stdio.h>
#include <string.h>
#include "stm32f7xx_hal.h"
#include "GLCD_Config.h"
#include "Board_GLCD.h"

// Level Files
#include "level_01.h" // int level_01_matrix[18][32]

// Definitions
#define wait HAL_Delay
extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;
typedef enum directions {UP, DOWN, LEFT, RIGHT} DirectionType;



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
	GLCD_SetFont(&GLCD_Font_16x24);
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
void DrawPlayer(int *playerGridPos) {
	GLCD_SetForegroundColor(GLCD_COLOR_YELLOW);
	Draw_Fill_Rect(playerGridPos[0]*15, playerGridPos[1]*15,30,30);
}

uint32_t ADC_VALUES[2];
int main(void) {
	
	// Player Variables
	int playerGridPos[2];
	DirectionType currentDirection;
	DirectionType requestedDirection;

	// Setup board
	setup();
	
	// Player starting position
	playerGridPos[0] = 1;
	playerGridPos[1] = 1;	
	
	// Start DMA for direct pipe from ADC
	CongigureDMA();
	HAL_ADC_Start_DMA(&hadc, ADC_VALUES, 2);
	
	// Draw level walls once
	GLCD_SetForegroundColor(GLCD_COLOR_BLUE);
	Draw_Level_Matrix(*level_01_matrix);

	// Game Loop
	while(1) {
		
		
		// Detect change in player input
		// Change direction value
		if(ADC_VALUES[0] > 3000) {
			requestedDirection = LEFT;
		}
		if(ADC_VALUES[0] < 1000) {
			requestedDirection = RIGHT;
		}
		if(ADC_VALUES[1] > 3000) {
			requestedDirection = UP;
		}
		if(ADC_VALUES[1] < 1000) {
			requestedDirection = DOWN;
		}
		//handleRequestedDirection(&currentDirection, requestedDirection, playerGridPos);
		
		
		// Draw Level Paths (Black won't apear to be flickering)
		//GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
		//Draw_Level_Path(*level_01_matrix, playerGridPos);
		
		wait(100);
		
		// Move pacman in current direction
		//MovePlayer(playerGridPos, level_01_matrix, currentDirection);
		// Draw updated pacman location
		//DrawPlayer(playerGridPos);
		
		
		
		//GLCD_ClearScreen();
	}
}
