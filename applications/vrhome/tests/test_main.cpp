#include "test.h"

int gChecks = 0, gFails = 0;

void testMat4();
void testHead();
void testPoseFilt();
void testLayout();
void testText();
void testSceneGeo();
void testKeys();
void testWarp();
void testDock();
void testShelf();
void testNotif();
void testSysMsg();
void testInput();
void testCtrlDebug();
void testStatus();
void testPalette();
void testPaletteThemes();
void testKbd();
void testGrid();
void testPt();
void testAnim();
void testZip();
void testEnvSel();
void testEnvMap();
void testQuiet();
void testPresent();

int main() {
    testMat4();
    testHead();
    testPoseFilt();
    testLayout();
    testText();
    testSceneGeo();
    testKeys();
    testWarp();
    testDock();
    testShelf();
    testNotif();
    testSysMsg();
    testInput();
    testCtrlDebug();
    testStatus();
    testPalette();
    testPaletteThemes();
    testKbd();
    testGrid();
    testPt();
    testAnim();
    testZip();
    testEnvSel();
    testEnvMap();
    testQuiet();
    testPresent();
    printf("%d checks, %d failures\n", gChecks, gFails);
    return gFails ? 1 : 0;
}
