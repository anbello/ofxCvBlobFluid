#pragma once

#include "MSAFluid.h"
//#include "MSATimer.h"
#include "ParticleSystem.h"

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxXmlSettings.h"
#include "ofxPostProcessing.h"

// comment this line out if you don't wanna use TUIO
// you will need ofxTUIO & ofxOsc
#define USE_TUIO

// comment this line out if you don't wanna use the GUI
// you will need ofxSimpleGuiToo, ofxMSAInteractiveObject & ofxXmlSettings
// if you don't use the GUI, you won't be able to see the fluid parameters
#define USE_GUI


#ifdef USE_TUIO
#include "ofxTuio.h"
#define tuioCursorSpeedMult				0.5	// the iphone screen is so small, easy to rack up huge velocities! need to scale down
#define tuioStationaryForce				0.001f	// force exerted when cursor is stationary
#endif


#ifdef USE_GUI
#include "ofxSimpleGuiToo.h"
#endif

class Glow : public ofxCv::RectFollower {
protected:
	ofColor color;
	ofVec2f cur, smooth, prev;
	float startedDying;
	ofPolyline all;
public:
    int numVert;

	Glow()
		:startedDying(0) {
	}
	void setup(const cv::Rect& track);
	void update(const cv::Rect& track);
	void kill();
	void draw();
	ofVec2f getPos();
	ofVec2f getVel();
};

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();

	void keyPressed  (int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);

	void fadeToColor(float r, float g, float b, float speed);
	void addToFluid(ofVec2f pos, ofVec2f vel, bool addColor, bool addForce);

    float                   colorMult;
    float                   velocityMult;
	int                     fluidCellsX;
	bool                    resizeFluid;
	bool                    drawFluid;
	bool                    drawParticles;

	msa::fluid::Solver      fluidSolver;
	msa::fluid::DrawerGl	fluidDrawer;

	ParticleSystem          particleSystem;

	ofVec2f                 pMouse;

#ifdef USE_TUIO
	ofxTuioClient tuioClient;
#endif

    // ================================================================
	// Blob
	// ================================================================

    ofxCv::ContourFinder contourFinder;
	ofxCv::RectTrackerFollower<Glow> tracker;

    ofxXmlSettings settings;

    ofVideoGrabber camera;
    ofImage camImg;
    float ofWidth;
    float ofHeight;
	int camWidth;
	int camHeight;
	int device;

	ofxPostProcessing post;
};
