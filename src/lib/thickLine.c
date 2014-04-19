/*
 * thickLine.c
 *
 * @date 25.03.2013
 * @author Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 * @copyright LGPL v3 (http://www.gnu.org/licenses/lgpl.html)
 * @version 1.5.0
 */

#include "thickline.h"
#include "HY32D.h"

/** @addtogroup Graphic_Library
 * @{
 */

/**
 * modified Bresenham
 */
void drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
	drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor);
}

/**
 * modified Bresenham with optional overlap (esp. for drawThickLine())
 * Overlap draws additional pixel when changing minor direction - for standard bresenham overlap = LINE_OVERLAP_NONE (0)
 *
 *  Sample line:
 *
 *    00+
 *     -0000+
 *         -0000+
 *             -00
 *
 *  0 pixels are drawn for normal line without any overlap
 *  + pixels are drawn if LINE_OVERLAP_MAJOR
 *  - pixels are drawn if LINE_OVERLAP_MINOR
 */
void drawLineOverlap(int16_t aXStart, int16_t aYStart, int16_t aXEnd, int16_t aYEnd, uint8_t aOverlap, uint16_t aColor) {
	int16_t tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

	/*
	 * Clip to display size
	 */
	if (aXStart >= DISPLAY_WIDTH) {
		aXStart = DISPLAY_WIDTH - 1;
	}
	if (aXStart < 0) {
		aXStart = 0;
	}
	if (aXEnd >= DISPLAY_WIDTH) {
		aXEnd = DISPLAY_WIDTH - 1;
	}
	if (aXEnd < 0) {
		aXEnd = 0;
	}
	if (aYStart >= DISPLAY_HEIGHT) {
		aYStart = DISPLAY_HEIGHT - 1;
	}
	if (aYStart < 0) {
		aYStart = 0;
	}
	if (aYEnd >= DISPLAY_HEIGHT) {
		aYEnd = DISPLAY_HEIGHT - 1;
	}
	if (aYEnd < 0) {
		aYEnd = 0;
	}

	if ((aXStart == aXEnd) || (aYStart == aYEnd)) {
		//horizontal or vertical line -> fillRect() is faster
		fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor);
	} else {
		//calculate direction
		tDeltaX = aXEnd - aXStart;
		tDeltaY = aYEnd - aYStart;
		if (tDeltaX < 0) {
			tDeltaX = -tDeltaX;
			tStepX = -1;
		} else {
			tStepX = +1;
		}
		if (tDeltaY < 0) {
			tDeltaY = -tDeltaY;
			tStepY = -1;
		} else {
			tStepY = +1;
		}
		tDeltaXTimes2 = tDeltaX << 1;
		tDeltaYTimes2 = tDeltaY << 1;
		//draw start pixel
		drawPixel(aXStart, aYStart, aColor);
		if (tDeltaX > tDeltaY) {
			// start value represents a half step in Y direction
			tError = tDeltaYTimes2 - tDeltaX;
			while (aXStart != aXEnd) {
				// step in main direction
				aXStart += tStepX;
				if (tError >= 0) {
					if (aOverlap & LINE_OVERLAP_MAJOR) {
						// draw pixel in main direction before changing
						drawPixel(aXStart, aYStart, aColor);
					}
					// change Y
					aYStart += tStepY;
					if (aOverlap & LINE_OVERLAP_MINOR) {
						// draw pixel in minor direction before changing
						drawPixel(aXStart - tStepX, aYStart, aColor);
					}
					tError -= tDeltaXTimes2;
				}
				tError += tDeltaYTimes2;
				drawPixel(aXStart, aYStart, aColor);
			}
		} else {
			tError = tDeltaXTimes2 - tDeltaY;
			while (aYStart != aYEnd) {
				aYStart += tStepY;
				if (tError >= 0) {
					if (aOverlap & LINE_OVERLAP_MAJOR) {
						// draw pixel in main direction before changing
						drawPixel(aXStart, aYStart, aColor);
					}
					aXStart += tStepX;
					if (aOverlap & LINE_OVERLAP_MINOR) {
						// draw pixel in minor direction before changing
						drawPixel(aXStart, aYStart - tStepY, aColor);
					}
					tError -= tDeltaYTimes2;
				}
				tError += tDeltaXTimes2;
				drawPixel(aXStart, aYStart, aColor);
			}
		}
	}
}

/**
 * Bresenham with thickness
 * no pixel missed and every pixel only drawn once!
 */
void drawThickLine(int16_t aXStart, int16_t aYStart, int16_t aXEnd, int16_t aYEnd, int16_t aThickness, uint8_t aThicknessMode,
		uint16_t aColor) {
	int16_t i, tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

	if(aThickness <= 1) {
		drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor);
	}
	/*
	 * Clip to display size
	 */
	if (aXStart >= DISPLAY_WIDTH) {
		aXStart = DISPLAY_WIDTH - 1;
	}
	if (aXStart < 0) {
		aXStart = 0;
	}
	if (aXEnd >= DISPLAY_WIDTH) {
		aXEnd = DISPLAY_WIDTH - 1;
	}
	if (aXEnd < 0) {
		aXEnd = 0;
	}
	if (aYStart >= DISPLAY_HEIGHT) {
		aYStart = DISPLAY_HEIGHT - 1;
	}
	if (aYStart < 0) {
		aYStart = 0;
	}
	if (aYEnd >= DISPLAY_HEIGHT) {
		aYEnd = DISPLAY_HEIGHT - 1;
	}
	if (aYEnd < 0) {
		aYEnd = 0;
	}

	/**
	 * For coordinate system with 0.0 top left
	 * Swap X and Y delta and calculate clockwise (new delta X inverted)
	 * or counterclockwise (new delta Y inverted) rectangular direction.
	 * The right rectangular direction for LINE_OVERLAP_MAJOR toggles with each octant
	 */
	tDeltaY = aXEnd - aXStart;
	tDeltaX = aYEnd - aYStart;
	// mirror 4 quadrants to one and adjust deltas and stepping direction
	bool tSwap = true; // count effective mirroring
	if (tDeltaX < 0) {
		tDeltaX = -tDeltaX;
		tStepX = -1;
		tSwap = !tSwap;
	} else {
		tStepX = +1;
	}
	if (tDeltaY < 0) {
		tDeltaY = -tDeltaY;
		tStepY = -1;
		tSwap = !tSwap;
	} else {
		tStepY = +1;
	}
	tDeltaXTimes2 = tDeltaX << 1;
	tDeltaYTimes2 = tDeltaY << 1;
	bool tOverlap;
	// adjust for right direction of thickness from line origin
	int tDrawStartAdjustCount = aThickness / 2;
	if (aThicknessMode == LINE_THICKNESS_DRAW_COUNTERCLOCKWISE) {
		tDrawStartAdjustCount = aThickness - 1;
	} else if (aThicknessMode == LINE_THICKNESS_DRAW_CLOCKWISE) {
		tDrawStartAdjustCount = 0;
	}

	// which octant are we now
	if (tDeltaX >= tDeltaY) {
		if (tSwap) {
			tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
			tStepY = -tStepY;
		} else {
			tStepX = -tStepX;
		}
		/*
		 * Vector for draw direction of lines is rectangular and counterclockwise to original line
		 * Therefore no pixel will be missed if LINE_OVERLAP_MAJOR is used
		 * on changing in minor rectangular direction
		 */
		// adjust draw start point
		tError = tDeltaYTimes2 - tDeltaX;
		for (i = tDrawStartAdjustCount; i > 0; i--) {
			// change X (main direction here)
			aXStart -= tStepX;
			aXEnd -= tStepX;
			if (tError >= 0) {
				// change Y
				aYStart -= tStepY;
				aYEnd -= tStepY;
				tError -= tDeltaXTimes2;
			}
			tError += tDeltaYTimes2;
		}
		//draw start line
		drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
		// draw aThickness lines
		tError = tDeltaYTimes2 - tDeltaX;
		for (i = aThickness; i > 1; i--) {
			// change X (main direction here)
			aXStart += tStepX;
			aXEnd += tStepX;
			tOverlap = LINE_OVERLAP_NONE;
			if (tError >= 0) {
				// change Y
				aYStart += tStepY;
				aYEnd += tStepY;
				tError -= tDeltaXTimes2;
				/*
				 * change in minor direction reverse to line (main) direction
				 * because of choosing the right (counter)clockwise draw vector
				 * use LINE_OVERLAP_MAJOR to fill all pixel
				 *
				 * EXAMPLE:
				 * 1,2 = Pixel of first lines
				 * 3 = Pixel of third line in normal line mode
				 * - = Pixel which will additionally be drawn in LINE_OVERLAP_MAJOR mode
				 *           33
				 *       3333-22
				 *   3333-222211
				 * 33-22221111
				 *  221111                     /\
				 *  11                          Main direction of draw vector
				 *  -> Line main direction
				 *  <- Minor direction of counterclockwise draw vector
				 */
				tOverlap = LINE_OVERLAP_MAJOR;
			}
			tError += tDeltaYTimes2;
			drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
		}
	} else {
		// the other octant
		if (tSwap) {
			tStepX = -tStepX;
		} else {
			tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
			tStepY = -tStepY;
		}
		// adjust draw start point
		tError = tDeltaXTimes2 - tDeltaY;
		for (i = tDrawStartAdjustCount; i > 0; i--) {
			aYStart -= tStepY;
			aYEnd -= tStepY;
			if (tError >= 0) {
				aXStart -= tStepX;
				aXEnd -= tStepX;
				tError -= tDeltaYTimes2;
			}
			tError += tDeltaXTimes2;
		}
		//draw start line
		drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
		tError = tDeltaXTimes2 - tDeltaY;
		for (i = aThickness; i > 1; i--) {
			aYStart += tStepY;
			aYEnd += tStepY;
			tOverlap = LINE_OVERLAP_NONE;
			if (tError >= 0) {
				aXStart += tStepX;
				aXEnd += tStepX;
				tError -= tDeltaYTimes2;
				tOverlap = LINE_OVERLAP_MAJOR;
			}
			tError += tDeltaXTimes2;
			drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
		}
	}
}
/**
 * The same as before, but no clipping to display range, some pixel are drawn twice (because of using LINE_OVERLAP_BOTH)
 * and direction of thickness changes for each octant (except for LINE_THICKNESS_MIDDLE and aThickness odd)
 */
void drawThickLineSimple(int16_t aXStart, int16_t aYStart, int16_t aXEnd, int16_t aYEnd, int16_t aThickness, uint8_t aThicknessMode,
		uint16_t aColor) {
	int16_t i, tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

	tDeltaY = aXStart - aXEnd;
	tDeltaX = aYEnd - aYStart;
	// mirror 4 quadrants to one and adjust deltas and stepping direction
	if (tDeltaX < 0) {
		tDeltaX = -tDeltaX;
		tStepX = -1;
	} else {
		tStepX = +1;
	}
	if (tDeltaY < 0) {
		tDeltaY = -tDeltaY;
		tStepY = -1;
	} else {
		tStepY = +1;
	}
	tDeltaXTimes2 = tDeltaX << 1;
	tDeltaYTimes2 = tDeltaY << 1;
	bool tOverlap;
	// which octant are we now
	if (tDeltaX > tDeltaY) {
		if (aThicknessMode == LINE_THICKNESS_MIDDLE) {
			// adjust draw start point
			tError = tDeltaYTimes2 - tDeltaX;
			for (i = aThickness / 2; i > 0; i--) {
				// change X (main direction here)
				aXStart -= tStepX;
				aXEnd -= tStepX;
				if (tError >= 0) {
					// change Y
					aYStart -= tStepY;
					aYEnd -= tStepY;
					tError -= tDeltaXTimes2;
				}
				tError += tDeltaYTimes2;
			}
		}
		//draw start line
		drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
		// draw aThickness lines
		tError = tDeltaYTimes2 - tDeltaX;
		for (i = aThickness; i > 1; i--) {
			// change X (main direction here)
			aXStart += tStepX;
			aXEnd += tStepX;
			tOverlap = LINE_OVERLAP_NONE;
			if (tError >= 0) {
				// change Y
				aYStart += tStepY;
				aYEnd += tStepY;
				tError -= tDeltaXTimes2;
				tOverlap = LINE_OVERLAP_BOTH;
			}
			tError += tDeltaYTimes2;
			drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
		}
	} else {
		// adjust draw start point
		if (aThicknessMode == LINE_THICKNESS_MIDDLE) {
			tError = tDeltaXTimes2 - tDeltaY;
			for (i = aThickness / 2; i > 0; i--) {
				aYStart -= tStepY;
				aYEnd -= tStepY;
				if (tError >= 0) {
					aXStart -= tStepX;
					aXEnd -= tStepX;
					tError -= tDeltaYTimes2;
				}
				tError += tDeltaXTimes2;
			}
		}
		//draw start line
		drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
		tError = tDeltaXTimes2 - tDeltaY;
		for (i = aThickness; i > 1; i--) {
			aYStart += tStepY;
			aYEnd += tStepY;
			tOverlap = LINE_OVERLAP_NONE;
			if (tError >= 0) {
				aXStart += tStepX;
				aXEnd += tStepX;
				tError -= tDeltaYTimes2;
				tOverlap = LINE_OVERLAP_BOTH;
			}
			tError += tDeltaXTimes2;
			drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
		}
	}
}

/**
 * aNewRelEndX + Y are new x and y values relative to start point
 */
void refreshVector(struct ThickLine * aLine, int16_t aNewRelEndX, int16_t aNewRelEndY) {
	int16_t tNewEndX = aLine->StartX + aNewRelEndX;
	int16_t tNewEndY = aLine->StartY + aNewRelEndY;
	if (aLine->EndX != tNewEndX || aLine->EndX != tNewEndY) {
		//clear old line
		drawThickLine(aLine->StartX, aLine->StartY, aLine->EndX, aLine->EndY, aLine->Thickness, aLine->ThicknessMode,
				aLine->BackgroundColor);
		// Draw new line
		/**
		 * clipping
		 */
		if (tNewEndX < 0) {
			tNewEndX = 0;
		} else if (tNewEndX > DISPLAY_WIDTH - 1) {
			tNewEndX = DISPLAY_WIDTH - 1;
		}
		aLine->EndX = tNewEndX;

		if (tNewEndY < 0) {
			tNewEndY = 0;
		} else if (tNewEndY > DISPLAY_HEIGHT - 1) {
			tNewEndY = DISPLAY_HEIGHT - 1;
		}
		aLine->EndY = tNewEndY;

		drawThickLine(aLine->StartX, aLine->StartY, tNewEndX, tNewEndY, aLine->Thickness, aLine->ThicknessMode, aLine->Color);
	}
}
/**
 * @}
 */
