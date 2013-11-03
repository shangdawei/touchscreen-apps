/**
 * PageFrequencyGenerator.cpp
 *
 * @date 26.10.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.0.0
 */

#include "Pages.h"
#include "misc.h"
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {

#include "stm32f30xPeripherals.h"
#include "timing.h"

}
#endif

/* Private define ------------------------------------------------------------*/

#define BACKGROUND_COLOR COLOR_WHITE
#define FREQ_SLIDER_SIZE 5 // size of border
#define FREQ_SLIDER_MAX_VALUE 300 /* size of slider */
// Start values for 10 kHz
#define START_FREQ_SLIDER_VALUE 100 // for 10
#define FREQ_START_FACTOR_INDEX 2 // for kHz
#define NUMBER_OF_FIXED_FREQUENCY_BUTTONS 7

#define INFO_Y_MARGIN 12

#define SLIDER_POS_X 5

/* Private variables ---------------------------------------------------------*/

static TouchButton * TouchButton1;
static TouchButton * TouchButton2;
static TouchButton * TouchButton5;
static TouchButton * TouchButton10;
static TouchButton * TouchButton20;
static TouchButton * TouchButton50;
static TouchButton * TouchButton100;
static TouchButton * TouchButtonx10;
static TouchButton * TouchButtonmHz;
static TouchButton * TouchButtonHz;
static TouchButton * TouchButtonkHz;
static TouchButton * TouchButtonMHz;

static TouchButton * TouchButtonStartStop;
static TouchButton * TouchButtonSetFrequency;
static TouchButton ** TouchButtonsFreqGen[] = { &TouchButton1, &TouchButton2, &TouchButton5, &TouchButton10, &TouchButton20,
		&TouchButton50, &TouchButton100, &TouchButtonx10, &TouchButtonmHz, &TouchButtonHz, &TouchButtonkHz, &TouchButtonMHz,
		&TouchButtonSetFrequency, &TouchButtonStartStop };

const uint16_t Frequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS + 3] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };
const char * FrequencyStrings[NUMBER_OF_FIXED_FREQUENCY_BUTTONS + 3] = { String1, String2, String5, String10, String20, String50,
		String100, String200, String500, String1000 };

const char Stringx10[] = "x10";
const char Stringx1[] = "x1";

static float sFrequency = 0;

const char FrequencyFactorChars[4] = { 'm', ' ', 'k', 'M' };
static uint8_t sFrequencyFactorIndex = FREQ_START_FACTOR_INDEX;
static uint32_t sFrequencyFactorTimes1000; // factor for mHz/Hz/kHz/MHz
static uint16_t sLastFrequencySliderValue = START_FREQ_SLIDER_VALUE;

static bool sFreqGenIsStopped = true;

/* Private function prototypes -----------------------------------------------*/
void drawFreqGenPage(void);
void setCapionAndDrawFixedFreqButtons(int aStartindex);
uint16_t doFrequencySlider(TouchSlider * const aTheTouchedSlider, uint16_t aValue);

/* Private functions ---------------------------------------------------------*/

void setFrequencyFactor(int aIndexValue) {
	sFrequencyFactorIndex = aIndexValue;
	int tFactor = 1;
	while (aIndexValue > 0) {
		tFactor *= 1000;
		aIndexValue--;
	}
	sFrequencyFactorTimes1000 = tFactor;
}

void initFreqGenPage(void) {
	HIRES_Timer_initialize(72000);
	setFrequencyFactor(sFrequencyFactorIndex);
	doFrequencySlider(NULL, sLastFrequencySliderValue);
}

/**
 * Computes Autoreload value for synthesizer
 * @param aSliderValue
 * @param aSetTimer
 */
// TODO check lower value for reload value
void ComputePeriodAndSetTimer(bool aSetSlider) {
	float tPeriod = (36000000000 / sFrequencyFactorTimes1000) / sFrequency; // 3600... because it is toggle mode
	uint32_t tPeriodInt = tPeriod;
	if (tPeriodInt < 2) {
		tPeriodInt = 2;
	}
	HIRES_Timer_SetReloadValue(tPeriodInt);
	// recompute exact frequency for given integer period
	tPeriod = tPeriodInt;
	float sFrequency = (36000000000 / sFrequencyFactorTimes1000) / tPeriod; // 3600... because it is toggle mode
	snprintf(StringBuffer, sizeof StringBuffer, "%9.4f%cHz", sFrequency, FrequencyFactorChars[sFrequencyFactorIndex]);
	drawText(SLIDER_POS_X + 2 * FONT_WIDTH, TouchSliderHorizontal.getPositionYBottom() + INFO_Y_MARGIN, StringBuffer, 1, COLOR_BLUE,
	BACKGROUND_COLOR);
	snprintf(StringBuffer, sizeof StringBuffer, "%#010lX", tPeriodInt);
	drawText(SLIDER_POS_X, BUTTON_HEIGHT_4_LINE_2 + BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING_HALF, StringBuffer, 1, COLOR_BLUE,
	BACKGROUND_COLOR);
	if (aSetSlider) {
		uint16_t tValue = log10f(sFrequency) * 100;
		TouchSliderHorizontal.setActualValueAndDrawBar(tValue);
	}
}

/**
 * 0 -> caption from 1-100 3-> caption 10-1000
 */
void setCapionAndDrawFixedFreqButtons(int aStartindex) {
// Fixed frequency buttons next
	int tXPos = 0;
	for (int i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
		(*TouchButtonsFreqGen[i])->setCaption(FrequencyStrings[i + aStartindex]);
		(*TouchButtonsFreqGen[i])->setValue(Frequency[i + aStartindex]);
		(*TouchButtonsFreqGen[i])->drawButton();
		tXPos = tXPos + BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF + 6;
	}
}

uint16_t doFrequencySlider(TouchSlider * const aTheTouchedSlider, uint16_t aValue) {
	float tValue = aValue;
	tValue = tValue / (FREQ_SLIDER_MAX_VALUE / 3); // gives 0-3
	sFrequency = pow10f(tValue);
	ComputePeriodAndSetTimer(false);
	return aValue;
}

/*
 * state is stored in button value (0 or 3)
 */
void doFreqX10(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	int tNewValue = 0;
	const char * tNewString = Stringx10;
	if (aValue == 0) {
		tNewValue = 3;
		tNewString = Stringx1;
	}
	aTheTouchedButton->setCaption(tNewString);
	aTheTouchedButton->setValue(tNewValue);
	aTheTouchedButton->drawButton();
	setCapionAndDrawFixedFreqButtons(tNewValue);
}

void doFreqGenStartStop(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	if (sFreqGenIsStopped) {
		aTheTouchedButton->setCaption(StringStop);
		HIRES_Timer_Start();
	} else {
		aTheTouchedButton->setCaption(StringStart);
		HIRES_Timer_Stop();
	}
	sFreqGenIsStopped = !sFreqGenIsStopped;
	aTheTouchedButton->drawButton();
}

void doSetFixedFrequency(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	sFrequency = aValue;
	ComputePeriodAndSetTimer(true);
}

void doChangeFrequencyFactor(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	setFrequencyFactor(aValue);
	ComputePeriodAndSetTimer(true);
}

void doSetFrequency(TouchButton * const aTheTouchedButton, int16_t aValue) {
	FeedbackToneOK();
	float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR_BLUE);
	if (!isnan(tNumber)) {
		sFrequency = tNumber;
	}
	drawFreqGenPage();
	ComputePeriodAndSetTimer(true);
}

// draw buttons
void drawFreqGenPage(void) {
	clearDisplay(COLOR_BACKGROUND_DEFAULT);
	TouchSliderHorizontal.drawSlider();
	drawChar(SLIDER_POS_X, TouchSliderHorizontal.getPositionYBottom() + 2, '1', 1, COLOR_BLUE, BACKGROUND_COLOR);
	drawText(TouchSliderHorizontal.getPositionXRight() - 4 * FONT_WIDTH, TouchSliderHorizontal.getPositionYBottom() + 2, String1000,
			1, COLOR_BLUE, BACKGROUND_COLOR);
	for (unsigned int i = 0; i < sizeof(TouchButtonsFreqGen) / sizeof(TouchButtonsFreqGen[0]); ++i) {
		(*TouchButtonsFreqGen[i])->drawButton();
	}
	TouchButtonMainHome->drawButton();
}

void startFreqGenPage(void) {
	// Frequency slider for 1 to 1000 at top of screen
	TouchSlider::setDefaultBarColor(TOUCHSLIDER_DEFAULT_BAR_COLOR);
	TouchSlider::setDefaultSliderColor(COLOR_BLUE);
	TouchSliderHorizontal.initSlider(SLIDER_POS_X, 5, FREQ_SLIDER_SIZE, FREQ_SLIDER_MAX_VALUE, (FREQ_SLIDER_MAX_VALUE / 3) * 2,
			sLastFrequencySliderValue, "Frequency", TOUCHSLIDER_DEFAULT_TOUCH_BORDER,
			TOUCHSLIDER_SHOW_BORDER | TOUCHSLIDER_IS_HORIZONTAL | TOUCHSLIDER_HORIZONTAL_VALUE_LEFT, &doFrequencySlider,
			NULL);

// Fixed frequency buttons next
	int tXPos = 0;
	for (int i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
		(*TouchButtonsFreqGen[i]) = TouchButton::allocAndInitSimpleButton(tXPos, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_8,
		BUTTON_HEIGHT_6, COLOR_BLUE, FrequencyStrings[i], 1, Frequency[i], &doSetFixedFrequency);
		tXPos = tXPos + BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF + 6;
	}

	// Factor next
	int tYPos = BUTTON_HEIGHT_4_LINE_2 + BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING_HALF;
	TouchButtonx10 = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, tYPos, BUTTON_WIDTH_3, BUTTON_HEIGHT_5,
	COLOR_BLUE, Stringx10, 2, 0, &doFreqX10);

	// Range next
	tXPos = 0;
	tYPos += BUTTON_HEIGHT_5 + BUTTON_DEFAULT_SPACING_HALF;
	TouchButtonmHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "mHz", 2, 0,
			&doChangeFrequencyFactor);
	tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
	TouchButtonHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "Hz", 2, 1,
			&doChangeFrequencyFactor);
	tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
	TouchButtonkHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "kHz", 2, 2,
			&doChangeFrequencyFactor);
	tXPos += BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING;
	TouchButtonMHz = TouchButton::allocAndInitSimpleButton(tXPos, tYPos, BUTTON_WIDTH_4, BUTTON_HEIGHT_5, COLOR_GREEN, "MHz", 2, 3,
			&doChangeFrequencyFactor);

	// Start + Frequency + Home last
	TouchButtonStartStop = TouchButton::allocAndInitSimpleButton(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
	COLOR_RED, StringStop, 2, 0, &doFreqGenStartStop);

	TouchButtonSetFrequency = TouchButton::allocAndInitSimpleButton(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4,
	BUTTON_WIDTH_3,
	BUTTON_HEIGHT_4, COLOR_BLUE, "Hz", 3, 0, &doSetFrequency);

	initMainHomeButtonWithPosition(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, false);
	drawFreqGenPage();
	setFrequencyFactor(sFrequencyFactorIndex);
	ComputePeriodAndSetTimer(true);
	HIRES_Timer_Start();
	sFreqGenIsStopped = false;
}

void loopFreqGenPage(void) {
//	drawInteger(0, BUTTON_HEIGHT_4_LINE_2 + BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING_HALF + FONT_HEIGHT, HIRES_Timer_GetCounter(),
//			16, 1, COLOR_BLACK, BACKGROUND_COLOR);
}

void stopFreqGenPage(void) {
// free buttons
	for (unsigned int i = 0; i < sizeof(TouchButtonsFreqGen) / sizeof(TouchButtonsFreqGen[0]); ++i) {
		(*TouchButtonsFreqGen[i])->setFree();
	}
}
