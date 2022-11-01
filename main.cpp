//==============================================================================
/*
    Yibo Wen
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------
#include "GL/glut.h"
#include "GEL3D.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> //for matrices
#include <glm/gtc/type_ptr.hpp>
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//------------------------------------------------------------------------------
// DECLARED CHAI3D VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cDirectionalLight* light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelHapticRate;

// a small sphere (cursor) representing the haptic device 
cShapeSphere* cursor;

// flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// flag to indicate if the haptic simulation has terminated
bool simulationFinished = false;

// frequency counter to measure the simulation haptic rate
cFrequencyCounter frequencyCounter;

// haptic thread
cThread* hapticsThread;

// information about computer screen and GLUT display window
int screenW;
int screenH;
int windowW;
int windowH;
int windowPosX;
int windowPosY;

//---------------------------------------------------------------------------
// GEL
//---------------------------------------------------------------------------

// deformable world
/*cGELWorld* defWorld;

// object mesh
cGELMesh* defObject;

// dynamic nodes
cGELSkeletonNode* nodes[10][10];

// haptic device model
cShapeSphere* device;
double deviceRadius;

// radius of the dynamic model sphere (GEM)
double radius;

// stiffness properties between the haptic device tool and the model (GEM)
double stiffness;*/

//------------------------------------------------------------------------------
// DECLARED GRAPHICS VARIABLES
//------------------------------------------------------------------------------

// grid size
int numX = 20, numY = 20;
const size_t total_points = (numX + 1) * (numY + 1);
float fullsize = 4.0f;
float halfsize = fullsize / 2.0f;

// FPS
float timeStep = 1 / 120.0f;
float currentTime = 0;
double accumulator = timeStep;
int selected_index = -1;
bool bDrawPoints = false;

struct Spring {
    int p1, p2;
    float rest_length;
    float Ks, Kd;
    int type;
};

vector<GLushort> indices;
vector<Spring> springs;

vector<glm::vec3> X;
vector<glm::vec3> V;
vector<glm::vec3> F;

int oldX = 0, oldY = 0;
float rX = 15, rY = 0;
int state = 1;
float dist = -15;
const int GRID_SIZE = 10;

const int STRUCTURAL_SPRING = 0;
const int SHEAR_SPRING = 1;
const int BEND_SPRING = 2;
int spring_count = 0;

const float DEFAULT_DAMPING = -0.1f;
float   KsStruct = 0.5f, KdStruct = -0.25f;
float   KsShear = 0.5f, KdShear = -0.25f;
float   KsBend = 0.85f, KdBend = -0.25f;
glm::vec3 gravity = glm::vec3(0.0f, -0.00981f, 0.0f);
float mass = 0.5f;

GLint viewport[4];
GLdouble MV[16];
GLdouble P[16];

glm::vec3 Up = glm::vec3(0, 1, 0), Right, viewDir;

LARGE_INTEGER frequency;        // ticks per second
LARGE_INTEGER t1, t2;           // ticks
double frameTimeQP = 0;
float frameTime = 0;

float startTime = 0, fps = 0;
int totalFrames = 0;

char info[MAX_PATH] = { 0 };

int iStacks = 30;
int iSlices = 30;
float fRadius = 1;

// Resolve constraint in object space
glm::vec3 center = glm::vec3(0, 0, 0); //object space center of ellipsoid
float radius = 1;                    //object space radius of ellipsoid


//------------------------------------------------------------------------------
// DECLARED CHAI3D FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void resizeWindow(int w, int h);

// callback when a key is pressed
void keySelect(unsigned char key, int x, int y);

// callback to render graphic scene
void updateGraphics(void);

// callback of GLUT timer
void graphicsTimer(int data);

// function that closes the application
void close(void);

// main haptics simulation loop
void updateHaptics(void);


//------------------------------------------------------------------------------
// DECLARED GRAPHICS FUNCTIONS
//------------------------------------------------------------------------------

// move to next state
void stepPhysics(float dt);

// add spring to the grid
void addSpring(int a, int b, float ks, float kd, int type);

// callback when mouse is down
void onMouseDown(int button, int s, int x, int y);

// callback when mouse is moved
void onMouseMove(int x, int y);

// draw lines of the grid
void drawGrid();

// initialize scene
void initGL();

// update cloth display in updateGraphics
void onRender();

// compute current forces
void computeForces();

// discrete step with euler
void integrateEuler(float deltaTime);

// apply dynamic inverse
void applyProvotDynamicInverse();

// callback when idle
void onIdle();

//==============================================================================
/*
    TEMPLATE: main.cpp

    Haptics rendering for deformable objects.
*/
//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable display points" << endl;
    cout << "[q] - Exit application" << endl;
    cout << endl << endl;


    //--------------------------------------------------------------------------
    // OPENGL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLUT
    glutInit(&argc, argv);

    // retrieve  resolution of computer display and position window accordingly
    screenW = glutGet(GLUT_SCREEN_WIDTH);
    screenH = glutGet(GLUT_SCREEN_HEIGHT);
    windowW = (int)(0.8 * screenH);
    windowH = (int)(0.5 * screenH);
    windowPosY = (screenH - windowH) / 2;
    windowPosX = windowPosY;

    // initialize the OpenGL GLUT window
    glutInitWindowPosition(windowPosX, windowPosY);
    glutInitWindowSize(windowW, windowH);

    if (stereoMode == C_STEREO_ACTIVE)
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO);
    else
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

    // create display context and initialize GLEW library
    glutCreateWindow("Cloth Demo [Explicit Euler Integration]");

#ifdef GLEW_VERSION
    // initialize GLEW
    glewInit();
#endif

    // setup GLUT options
    glutDisplayFunc(updateGraphics);
    glutReshapeFunc(resizeWindow);
    glutIdleFunc(onIdle);

    glutMouseFunc(onMouseDown);
    glutMotionFunc(onMouseMove);
    glutKeyboardFunc(keySelect);
    glutSetWindowTitle("Deformable Objects");

    // set fullscreen mode
    if (fullscreen)
    {
        glutFullScreen();
    }


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setBlack();

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set(cVector3d(0.5, 0.0, 0.0),    // camera position (eye)
        cVector3d(0.0, 0.0, 0.0),    // look at position (target)
        cVector3d(0.0, 0.0, 1.0));   // direction of the (up) vector

// set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 100.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.01);
    camera->setStereoFocalLength(0.5);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a directional light source
    light = new cDirectionalLight(world);

    // insert light source inside world
    world->addChild(light);

    // enable light source
    light->setEnabled(true);

    // define direction of light beam
    light->setDir(-1.0, 0.0, 0.0);

    // create a sphere (cursor) to represent the haptic device
    cursor = new cShapeSphere(0.01);

    // insert cursor inside world
    world->addChild(cursor);

    // initialize scene
    initGL();


    //--------------------------------------------------------------------------
    // HAPTIC DEVICE
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get a handle to the first haptic device
    handler->getDevice(hapticDevice, 0);

    // open a connection to haptic device
    hapticDevice->open();

    // calibrate device (if necessary)
    hapticDevice->calibrate();

    // retrieve information about the current haptic device
    cHapticDeviceInfo info = hapticDevice->getSpecifications();

    // display a reference frame if haptic device supports orientations
    if (info.m_sensedRotation == true)
    {
        // display reference frame
        cursor->setShowFrame(true);

        // set the size of the reference frame
        cursor->setFrameSize(0.05);
    }

    // if the device has a gripper, enable the gripper to simulate a user switch
    hapticDevice->setEnableGripperUserSwitch(true);


    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    cFontPtr font = NEW_CFONTCALIBRI20();

    // create a label to display the haptic rate of the simulation
    labelHapticRate = new cLabel(font);
    labelHapticRate->m_fontColor.setWhite();
    camera->m_frontLayer->addChild(labelHapticRate);


    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);

    // start the main graphics rendering loop
    glutTimerFunc(50, graphicsTimer, 0);
    glutMainLoop();

    // exit
    return (0);
}

//------------------------------------------------------------------------------

void resizeWindow(int w, int h)
{
    windowW = w;
    windowH = h;
}

//------------------------------------------------------------------------------

void keySelect(unsigned char key, int x, int y)
{
    // option ESC: exit
    if ((key == 27) || (key == 'q'))
    {
        close();
        exit(0);
    }

    // option f: toggle fullscreen
    if (key == 'f')
    {
        if (fullscreen)
        {
            windowPosX = glutGet(GLUT_INIT_WINDOW_X);
            windowPosY = glutGet(GLUT_INIT_WINDOW_Y);
            windowW = glutGet(GLUT_INIT_WINDOW_WIDTH);
            windowH = glutGet(GLUT_INIT_WINDOW_HEIGHT);
            glutPositionWindow(windowPosX, windowPosY);
            glutReshapeWindow(windowW, windowH);
            fullscreen = false;
        }
        else
        {
            glutFullScreen();
            fullscreen = true;
        }
    }

    // option m: toggle display of points
    if (key == 'm')
    {
        bDrawPoints = !bDrawPoints;
        glutPostRedisplay();
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    hapticDevice->close();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;

    // clear graphics simulation
    X.clear();
    V.clear();
    F.clear();

    indices.clear();
    springs.clear();
}

//------------------------------------------------------------------------------

void graphicsTimer(int data)
{
    if (simulationRunning)
    {
        glutPostRedisplay();
    }

    glutTimerFunc(50, graphicsTimer, 0);
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    // display haptic rate data
    labelHapticRate->setText(cStr(frequencyCounter.getFrequency(), 0) + " Hz");

    // update position of label
    labelHapticRate->setLocalPos((int)(0.5 * (windowW - labelHapticRate->getWidth())), 15);


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    camera->renderView(windowW, windowH);

    // render cloth
    onRender();

    // swap buffers
    glutSwapBuffers();

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//------------------------------------------------------------------------------

void updateHaptics(void)
{
    // initialize precision clock
    /*cPrecisionClock clock;
    clock.reset();

    // simulation in now running
    simulationRunning = true;
    simulationFinished = false;

    // main haptic simulation loop
    while (simulationRunning)
    {
        // stop clock
        double time = cMin(0.001, clock.stop());

        // restart clock
        clock.start(true);

        // read position from haptic device
        cVector3d pos;
        hapticDevice->getPosition(pos);
        pos.mul(workspaceScaleFactor);
        device->setLocalPos(pos);

        // clear all external forces
        defWorld->clearExternalForces();

        // compute reaction forces
        cVector3d force(0.0, 0.0, 0.0);
        for (int y = 0; y < 10; y++)
        {
            for (int x = 0; x < 10; x++)
            {
                cVector3d nodePos = nodes[x][y]->m_pos;
                cVector3d f = computeForce(pos, deviceRadius, nodePos, radius, stiffness);
                cVector3d tmpfrc = -1.0 * f;
                nodes[x][y]->setExternalForce(tmpfrc);
                force.add(f);
            }
        }

        // integrate dynamics
        defWorld->updateDynamics(time);

        // scale force
        force.mul(deviceForceScale / workspaceScaleFactor);

        // send forces to haptic device
        hapticDevice->setForce(force);

        // signal frequency counter
        freqCounterHaptics.signal(1);
    }

    // exit haptics thread
    simulationFinished = true;*/


    // initialize frequency counter
    frequencyCounter.reset();

    // simulation in now running
    simulationRunning = true;
    simulationFinished = false;

    // main haptic simulation loop
    while (simulationRunning)
    {
        /////////////////////////////////////////////////////////////////////
        // READ HAPTIC DEVICE
        /////////////////////////////////////////////////////////////////////

        // read position 
        cVector3d position;
        hapticDevice->getPosition(position);

        // read orientation 
        cMatrix3d rotation;
        hapticDevice->getRotation(rotation);

        // read user-switch status (button 0)
        bool button = false;
        hapticDevice->getUserSwitch(0, button);


        /////////////////////////////////////////////////////////////////////
        // UPDATE 3D CURSOR MODEL
        /////////////////////////////////////////////////////////////////////

        // update position and orienation of cursor
        cursor->setLocalPos(position);
        cursor->setLocalRot(rotation);

        /////////////////////////////////////////////////////////////////////
        // COMPUTE FORCES
        /////////////////////////////////////////////////////////////////////

        cVector3d force(0, 0, 0);
        cVector3d torque(0, 0, 0);
        double gripperForce = 0.0;


        /////////////////////////////////////////////////////////////////////
        // APPLY FORCES
        /////////////////////////////////////////////////////////////////////

        // send computed force, torque, and gripper force to haptic device
        hapticDevice->setForceAndTorqueAndGripperForce(force, torque, gripperForce);

        // update frequency counter
        frequencyCounter.signal(1);
    }

    // exit haptics thread
    simulationFinished = true;
}

//------------------------------------------------------------------------------

void onMouseDown(int button, int s, int x, int y)
{
    if (s == GLUT_DOWN)
    {
        oldX = x;
        oldY = y;
        int window_y = (windowH - y);
        float norm_y = float(window_y) / float(windowH / 2.0);
        int window_x = x;
        float norm_x = float(window_x) / float(windowW / 2.0);

        float winZ = 0;
        glReadPixels(x, windowH - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
        if (winZ == 1)
            winZ = 0;
        double objX = 0, objY = 0, objZ = 0;
        gluUnProject(window_x, window_y, winZ, MV, P, viewport, &objX, &objY, &objZ);
        glm::vec3 pt(objX, objY, objZ);
        size_t i = 0;
        for (i = 0; i < total_points; i++) {
            if (glm::distance(X[i], pt) < 0.1) {
                selected_index = i;
                printf("Intersected at %d\n", i);
                break;
            }
        }
    }

    if (button == GLUT_MIDDLE_BUTTON)
        state = 0;
    else
        state = 1;

    if (s == GLUT_UP) {
        selected_index = -1;
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }
}

//------------------------------------------------------------------------------

void onMouseMove(int x, int y)
{
    if (selected_index == -1) {
        if (state == 0)
            dist *= (1 + (y - oldY) / 60.0f);
        else
        {
            rY += (x - oldX) / 5.0f;
            rX += (y - oldY) / 5.0f;
        }
    }
    else {
        float delta = 1500 / abs(dist);
        float valX = (x - oldX) / delta;
        float valY = (oldY - y) / delta;
        if (abs(valX) > abs(valY))
            glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
        else
            glutSetCursor(GLUT_CURSOR_UP_DOWN);
        V[selected_index] = glm::vec3(0);
        X[selected_index].x += Right[0] * valX;
        float newValue = X[selected_index].y + Up[1] * valY;
        if (newValue > 0)
            X[selected_index].y = newValue;
        X[selected_index].z += Right[2] * valX + Up[2] * valY;

        //V[selected_index].x = 0;
        //V[selected_index].y = 0;
        //V[selected_index].z = 0;
    }
    oldX = x;
    oldY = y;

    glutPostRedisplay();
}

//------------------------------------------------------------------------------

void onIdle() {
    //Fixed time stepping + rendering at different fps
    if (accumulator >= timeStep)
    {
        stepPhysics(timeStep);
        accumulator -= timeStep;
    }
    glutPostRedisplay();
}

//------------------------------------------------------------------------------

void stepPhysics(float dt) {
    computeForces();

    //for Explicit Euler
    integrateEuler(dt);

    applyProvotDynamicInverse();
}

//------------------------------------------------------------------------------

void computeForces() {
    size_t i = 0;
    /*std::cout << "points " << total_points << std::endl;
    std::cout << "x " << numX << std::endl;*/
    for (i = 0; i < total_points; i++) {
        F[i] = glm::vec3(0);

        //add gravity force only for non-fixed points
        if (i != 0 && i != numX && i != 440 && i != 420)
            F[i] += gravity;

        //add force due to damping of velocity

        F[i] += DEFAULT_DAMPING * V[i];
    }

    //add spring forces
    for (i = 0; i < springs.size(); i++) {
        glm::vec3 p1 = X[springs[i].p1];
        glm::vec3 p2 = X[springs[i].p2];
        glm::vec3 v1 = V[springs[i].p1];
        glm::vec3 v2 = V[springs[i].p2];
        glm::vec3 deltaP = p1 - p2;
        glm::vec3 deltaV = v1 - v2;
        float dist = glm::length(deltaP);

        float leftTerm = -springs[i].Ks * (dist - springs[i].rest_length);
        float rightTerm = springs[i].Kd * (glm::dot(deltaV, deltaP) / dist);
        glm::vec3 springForce = (leftTerm + rightTerm) * glm::normalize(deltaP);

        if (springs[i].p1 != 0 && springs[i].p1 != numX && springs[i].p1 != 440 && springs[i].p1 != 420)
            F[springs[i].p1] += springForce;
        if (springs[i].p2 != 0 && springs[i].p2 != numX && springs[i].p2 != 440 && springs[i].p2 != 420)
            F[springs[i].p2] -= springForce;
    }
}

//------------------------------------------------------------------------------

void integrateEuler(float deltaTime) {
    float deltaTimeMass = deltaTime / mass;
    size_t i = 0;

    for (i = 0; i < total_points; i++) {
        glm::vec3 oldV = V[i];
        
        if (i != 0 && i != numX && i != 440 && i != 420) {
            V[i] += (F[i] * deltaTimeMass);
            X[i] += deltaTime * oldV;
        }
        
        if (X[i].y < 0) {
            X[i].y = 0;
        }
    }
}

//------------------------------------------------------------------------------

void applyProvotDynamicInverse() {

    for (size_t i = 0; i < springs.size(); i++) {
        //check the current lengths of all springs
        glm::vec3 p1 = X[springs[i].p1];
        glm::vec3 p2 = X[springs[i].p2];
        glm::vec3 deltaP = p1 - p2;
        float dist = glm::length(deltaP);
        if (dist > springs[i].rest_length) {
            dist -= (springs[i].rest_length);
            dist /= 2.0f;
            deltaP = glm::normalize(deltaP);
            deltaP *= dist;
            if (springs[i].p1 == 0 || springs[i].p1 == numX || springs[i].p1 == 440 || springs[i].p1 == 420) {
                V[springs[i].p2] += deltaP;
            }
            else if (springs[i].p2 == 0 || springs[i].p2 == numX || springs[i].p2 == 440 || springs[i].p2 == 420) {
                V[springs[i].p1] -= deltaP;
            }
            else {
                V[springs[i].p1] -= deltaP;
                V[springs[i].p2] += deltaP;
            }
        }
    }
}

//------------------------------------------------------------------------------

void addSpring(int a, int b, float ks, float kd, int type) {
    Spring spring;
    spring.p1 = a;
    spring.p2 = b;
    spring.Ks = ks;
    spring.Kd = kd;
    spring.type = type;
    glm::vec3 deltaP = X[a] - X[b];
    spring.rest_length = sqrt(glm::dot(deltaP, deltaP));
    springs.push_back(spring);
}

//------------------------------------------------------------------------------

void drawGrid()
{
    glBegin(GL_LINES);
    glColor3f(0.5f, 0.5f, 0.5f);
    for (int i = -GRID_SIZE; i <= GRID_SIZE; i++)
    {
        glVertex3f((float)i, 0, (float)-GRID_SIZE);
        glVertex3f((float)i, 0, (float)GRID_SIZE);

        glVertex3f((float)-GRID_SIZE, 0, (float)i);
        glVertex3f((float)GRID_SIZE, 0, (float)i);
    }
    glEnd();
}

//------------------------------------------------------------------------------

void initGL() {
    startTime = (float)glutGet(GLUT_ELAPSED_TIME);
    currentTime = startTime;

    // get ticks per second
    QueryPerformanceFrequency(&frequency);

    // start timer
    QueryPerformanceCounter(&t1);

    glEnable(GL_DEPTH_TEST);
    int i = 0, j = 0, count = 0;
    int l1 = 0, l2 = 0;
    float ypos = 7.0f;
    int v = numY + 1;
    int u = numX + 1;

    indices.resize(numX * numY * 2 * 3);
    X.resize(total_points);
    V.resize(total_points);
    F.resize(total_points);

    // fill in X
    for (j = 0; j < v; j++) {
        for (i = 0; i < u; i++) {
            //X[count++] = glm::vec3( ((float(i)/(u-1)) *2-1)* halfsize, fullsize+1, ((float(j)/(v-1) )* fullsize));
            X[count++] = glm::vec3(((float(i) / (u - 1)) * 2 - 1) * halfsize, 2.0f, ((float(j) / (v - 1)) * fullsize));
        }
    }

    // fill in V
    memset(&(V[0].x), 0, total_points * sizeof(glm::vec3));

    // fill in indices
    GLushort* id = &indices[0];
    for (i = 0; i < numY; i++) {
        for (j = 0; j < numX; j++) {
            int i0 = i * (numX + 1) + j;
            int i1 = i0 + 1;
            int i2 = i0 + (numX + 1);
            int i3 = i2 + 1;
            if ((j + i) % 2) {
                *id++ = i0; *id++ = i2; *id++ = i1;
                *id++ = i1; *id++ = i2; *id++ = i3;
            }
            else {
                *id++ = i0; *id++ = i2; *id++ = i3;
                *id++ = i0; *id++ = i3; *id++ = i1;
            }
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPointSize(5);

    //wglSwapIntervalEXT(0);

    // setup springs
    // Horizontal
    for (l1 = 0; l1 < v; l1++)  // v
        for (l2 = 0; l2 < (u - 1); l2++) {
            addSpring((l1 * u) + l2, (l1 * u) + l2 + 1, KsStruct, KdStruct, STRUCTURAL_SPRING);
        }

    // Vertical
    for (l1 = 0; l1 < (u); l1++)
        for (l2 = 0; l2 < (v - 1); l2++) {
            addSpring((l2 * u) + l1, ((l2 + 1) * u) + l1, KsStruct, KdStruct, STRUCTURAL_SPRING);
        }

    // Shearing Springs
    for (l1 = 0; l1 < (v - 1); l1++)
        for (l2 = 0; l2 < (u - 1); l2++) {
            addSpring((l1 * u) + l2, ((l1 + 1) * u) + l2 + 1, KsShear, KdShear, SHEAR_SPRING);
            addSpring(((l1 + 1) * u) + l2, (l1 * u) + l2 + 1, KsShear, KdShear, SHEAR_SPRING);
        }

    // Bend Springs
    for (l1 = 0; l1 < (v); l1++) {
        for (l2 = 0; l2 < (u - 2); l2++) {
            addSpring((l1 * u) + l2, (l1 * u) + l2 + 2, KsBend, KdBend, BEND_SPRING);
        }
        addSpring((l1 * u) + (u - 3), (l1 * u) + (u - 1), KsBend, KdBend, BEND_SPRING);
    }
    for (l1 = 0; l1 < (u); l1++) {
        for (l2 = 0; l2 < (v - 2); l2++) {
            addSpring((l2 * u) + l1, ((l2 + 2) * u) + l1, KsBend, KdBend, BEND_SPRING);
        }
        addSpring(((v - 3) * u) + l1, ((v - 1) * u) + l1, KsBend, KdBend, BEND_SPRING);
    }
}

//------------------------------------------------------------------------------

void onRender() {
    size_t i = 0;
    float newTime = (float)glutGet(GLUT_ELAPSED_TIME);
    frameTime = newTime - currentTime;
    currentTime = newTime;
    //accumulator += frameTime;

    //Using high res. counter
    QueryPerformanceCounter(&t2);
    // compute and print the elapsed time in millisec
    frameTimeQP = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    t1 = t2;
    accumulator += frameTimeQP;

    ++totalFrames;
    if ((newTime - startTime) > 1000)
    {
        float elapsedTime = (newTime - startTime);
        fps = (totalFrames / elapsedTime) * 1000;
        startTime = newTime;
        totalFrames = 0;
    }

    sprintf_s(info, "FPS: %3.2f, Frame time (GLUT): %3.4f msecs, Frame time (QP): %3.3f", fps, frameTime, frameTimeQP);
    glutSetWindowTitle(info);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0, 0, dist);
    glRotatef(rX, 1, 0, 0);
    glRotatef(rY, 0, 1, 0);

    glGetDoublev(GL_MODELVIEW_MATRIX, MV);
    viewDir.x = (float)-MV[2];
    viewDir.y = (float)-MV[6];
    viewDir.z = (float)-MV[10];
    Right = glm::cross(viewDir, Up);

    //draw grid
    drawGrid();

    //draw polygons
    glColor3f(1, 1, 1);
    glBegin(GL_TRIANGLES);
    for (i = 0; i < indices.size(); i += 3) {
        glm::vec3 p1 = X[indices[i]];
        glm::vec3 p2 = X[indices[i + 1]];
        glm::vec3 p3 = X[indices[i + 2]];
        glVertex3f(p1.x, p1.y, p1.z);
        glVertex3f(p2.x, p2.y, p2.z);
        glVertex3f(p3.x, p3.y, p3.z);
    }
    glEnd();

    //draw points
    if (bDrawPoints) {
        glBegin(GL_POINTS);
        for (i = 0; i < total_points; i++) {
            glm::vec3 p = X[i];
            int is = (i == selected_index);
            glColor3f((float)!is, (float)is, (float)is);
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }
}