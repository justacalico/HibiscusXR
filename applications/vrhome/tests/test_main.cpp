#include "test.h"

int gChecks = 0, gFails = 0;

void testMat4();
void testHead();
void testLayout();
void testText();
void testSceneGeo();
void testKeys();
void testWarp();
void testDock();
void testShelf();
void testNotif();
void testInput();

int main() {
    testMat4();
    testHead();
    testLayout();
    testText();
    testSceneGeo();
    testKeys();
    testWarp();
    testDock();
    testShelf();
    testNotif();
    testInput();
    printf("%d checks, %d failures\n", gChecks, gFails);
    return gFails ? 1 : 0;
}
