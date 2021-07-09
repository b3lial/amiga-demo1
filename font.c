#include "demo1.h"

/**
 * This data highly depends on the font
 */
void getCharData(char letter, struct CharBlob *charBlob)
{
    letter = tolower(letter);

    switch (letter)
    {
    case 'a':
        charBlob->xSize = 28;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 0;
        break;
    case 'b':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 0;
        break;
    case 'c':
        charBlob->xSize = 22;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 0;
        break;
    case 'd':
        charBlob->xSize = 25;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 0;
        break;
    case 'e':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 0;
        break;
    case 'f':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 0;
        break;
    case 'g':
        charBlob->xSize = 24;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 241;
        charBlob->yPosInFont = 0;
        break;
    case 'h':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 40;
        break;
    case 'i':
        charBlob->xSize = 5;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 40;
        break;
    case 'j':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 40;
        break;
    case 'k':
        charBlob->xSize = 18;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 40;
        break;
    case 'l':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 40;
        break;
    case 'm':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 40;
        break;
    case 'n':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 241;
        charBlob->yPosInFont = 40;
        break;
    case 'o':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 80;
        break;
    case 'p':
        charBlob->xSize = 23;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 80;
        break;
    case 'q':
        charBlob->xSize = 30;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 80;
        break;
    case 'r':
        charBlob->xSize = 23;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 80;
        break;
    case 's':
        charBlob->xSize = 22;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 80;
        break;
    case 't':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 80;
        break;
    case 'u':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 241;
        charBlob->yPosInFont = 80;
        break;
    case 'v':
        charBlob->xSize = 25;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 120;
        break;
    case 'w':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 120;
        break;
    case 'x':
        charBlob->xSize = 25;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 120;
        break;
    case 'y':
        charBlob->xSize = 26;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 120;
        break;
    case 'z':
        charBlob->xSize = 20;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 120;
        break;
    case ' ':
        charBlob->xSize = 12;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 120;
        break;
    default:
        charBlob->xSize = 12;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 120;
        break;
    }
}