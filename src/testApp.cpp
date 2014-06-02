#include "testApp.h"

using namespace ofxCv;
using namespace cv;

const float dyingTime = 1;

void Glow::setup(const cv::Rect& track) {
	color.setHsb(ofRandom(0, 255), 255, 255);
	cur = toOf(track).getCenter();
	smooth = cur;
	prev = cur;
	numVert = 200;
}

void Glow::update(const cv::Rect& track) {
    if(all.size() > 0) {
	    vector<ofPoint> pts;
		pts = all.getVertices();
		prev = pts.back();
	}

	cur = toOf(track).getCenter();
	smooth.interpolate(cur, .5);
	all.addVertex(smooth);

	if(all.size() > numVert) {
	    vector<ofPoint> pts;
		pts = all.getVertices();
		pts.erase(pts.begin());
		all.clear();
		all.addVertices(pts);
	}
}

void Glow::kill() {
	float curTime = ofGetElapsedTimef();
	if(startedDying == 0) {
		startedDying = curTime;
//	} else if(curTime - startedDying > dyingTime) {
//		dead = true;
	} else if(all.size() > 0) {
	    vector<ofPoint> pts;
		pts = all.getVertices();
		pts.erase(pts.begin());
		all.clear();
		all.addVertices(pts);
	} else {
	    dead = true;
	}
}

void Glow::draw() {
	ofPushStyle();
	float size = 16;
//	ofSetColor(255);
//	if(startedDying) {
//		ofSetColor(ofColor::red);
//		size = ofMap(ofGetElapsedTimef() - startedDying, 0, dyingTime, size, 0, true);
//	}
//	ofNoFill();
//	ofCircle(cur, size);
	ofSetColor(color);
	ofSetLineWidth(5);
	all.draw();
//	ofSetColor(255);
//	ofDrawBitmapString(ofToString(label), cur);
	ofPopStyle();
}

ofVec2f Glow::getPos() {
    return smooth;
}

ofVec2f Glow::getVel() {
    return (smooth - prev);
}


char sz[] = "[Rd9?-2XaUP0QY[hO%9QTYQ`-W`QZhcccYQY[`b";

float tuioXScaler = 1;
float tuioYScaler = 1;

//--------------------------------------------------------------
void testApp::setup() {
	for(int i=0; i<strlen(sz); i++) sz[i] += 20;

	// setup fluid stuff
	fluidSolver.setup(100, 100);
    fluidSolver.enableRGB(true).setFadeSpeed(0.002).setDeltaT(0.5).setVisc(0.00015).setColorDiffusion(0);
	fluidDrawer.setup(&fluidSolver);

	fluidCellsX			= 150;

	drawFluid			= true;
	drawParticles		= true;

	//ofSetFrameRate(60);
	ofBackground(0, 0, 0);
	ofSetVerticalSync(true);

#ifdef USE_TUIO
	tuioClient.start(3333);
#endif


#ifdef USE_GUI
	gui.addSlider("fluidCellsX", fluidCellsX, 20, 400);
	gui.addButton("resizeFluid", resizeFluid);
    gui.addSlider("colorMult", colorMult, 0, 100);
    gui.addSlider("velocityMult", velocityMult, 0, 100);
	gui.addSlider("fs.viscocity", fluidSolver.viscocity, 0.0, 0.01);
	gui.addSlider("fs.colorDiffusion", fluidSolver.colorDiffusion, 0.0, 0.0003);
	gui.addSlider("fs.fadeSpeed", fluidSolver.fadeSpeed, 0.0, 0.1);
	gui.addSlider("fs.solverIterations", fluidSolver.solverIterations, 1, 50);
	gui.addSlider("fs.deltaT", fluidSolver.deltaT, 0.1, 5);
	gui.addComboBox("fd.drawMode", (int&)fluidDrawer.drawMode, msa::fluid::getDrawModeTitles());
	gui.addToggle("fs.doRGB", fluidSolver.doRGB);
	gui.addToggle("fs.doVorticityConfinement", fluidSolver.doVorticityConfinement);
	gui.addToggle("drawFluid", drawFluid);
	gui.addToggle("drawParticles", drawParticles);
	gui.addToggle("fs.wrapX", fluidSolver.wrap_x);
	gui.addToggle("fs.wrapY", fluidSolver.wrap_y);
    gui.addSlider("tuioXScaler", tuioXScaler, 0, 2);
    gui.addSlider("tuioYScaler", tuioYScaler, 0, 2);

	gui.currentPage().setXMLName("ofxMSAFluidSettings.xml");
    gui.loadFromXML();
	gui.setDefaultKeys(true);
	gui.setAutoSave(true);
    gui.show();
#endif

	windowResized(ofGetWidth(), ofGetHeight());		// force this at start (cos I don't think it is called)
	pMouse = msa::getWindowCenter();
	resizeFluid			= true;

	ofEnableAlphaBlending();
	ofSetBackgroundAuto(true);
	ofEnableAntiAliasing();

	// ================================================================
	// Blob
	// ================================================================

	settings.loadFile("settings.xml");
    camWidth = settings.getValue("settings:width", 640);
    camHeight = settings.getValue("settings:height", 480);
    device = settings.getValue("settings:device", 1);

	camera.setVerbose(true);
	camera.listDevices();
	camera.setDeviceID(device);
	camera.setDesiredFrameRate(30);
	camera.initGrabber(camWidth, camHeight);

	camImg.allocate(camWidth, camHeight, OF_IMAGE_COLOR);

	contourFinder.setMinAreaRadius(1);
	contourFinder.setMaxAreaRadius(50);
	contourFinder.setThreshold(190);

	// wait for half a frame before forgetting something
	tracker.setPersistence(15);
	// an object can move up to 32 pixels per frame
	tracker.setMaximumDistance(32);

	post.init(ofGetWidth(), ofGetHeight());
	post.createPass<FxaaPass>()->setEnabled(false);
    post.createPass<BloomPass>()->setEnabled(false);
    post.createPass<DofPass>()->setEnabled(false);
    post.createPass<ToonPass>()->setEnabled(false);
    post.createPass<BleachBypassPass>()->setEnabled(false);
    post.createPass<EdgePass>()->setEnabled(false);

    post.setFlip(false);
}


void testApp::fadeToColor(float r, float g, float b, float speed) {
    glColor4f(r, g, b, speed);
	ofRect(0, 0, ofGetWidth(), ofGetHeight());
}


// add force and dye to fluid, and create particles
void testApp::addToFluid(ofVec2f pos, ofVec2f vel, bool addColor, bool addForce) {
    float speed = vel.x * vel.x  + vel.y * vel.y * msa::getWindowAspectRatio() * msa::getWindowAspectRatio();    // balance the x and y components of speed with the screen aspect ratio
    if(speed > 0) {
		pos.x = ofClamp(pos.x, 0.0f, 1.0f);
		pos.y = ofClamp(pos.y, 0.0f, 1.0f);

        int index = fluidSolver.getIndexForPos(pos);

		if(addColor) {
//			Color drawColor(CM_HSV, (getElapsedFrames() % 360) / 360.0f, 1, 1);
			ofColor drawColor;
			drawColor.setHsb((ofGetFrameNum() % 255), 255, 255);

			fluidSolver.addColorAtIndex(index, drawColor * colorMult);

			if(drawParticles)
				particleSystem.addParticles(pos * ofVec2f(ofGetWindowSize()), 10);
		}

		if(addForce)
			fluidSolver.addForceAtIndex(index, vel * velocityMult);

    }
}


void testApp::update(){
	if(resizeFluid) 	{
		fluidSolver.setSize(fluidCellsX, fluidCellsX / msa::getWindowAspectRatio());
		fluidDrawer.setup(&fluidSolver);
		resizeFluid = false;
	}

#ifdef USE_TUIO
	tuioClient.getMessage();

	// do finger stuff
	list<ofxTuioCursor*>cursorList = tuioClient.getTuioCursors();
	for(list<ofxTuioCursor*>::iterator it=cursorList.begin(); it != cursorList.end(); it++) {
		ofxTuioCursor *tcur = (*it);
        float vx = tcur->getXSpeed() * tuioCursorSpeedMult;
        float vy = tcur->getYSpeed() * tuioCursorSpeedMult;
        if(vx == 0 && vy == 0) {
            vx = ofRandom(-tuioStationaryForce, tuioStationaryForce);
            vy = ofRandom(-tuioStationaryForce, tuioStationaryForce);
        }
        addToFluid(ofVec2f(tcur->getX() * tuioXScaler, tcur->getY() * tuioYScaler), ofVec2f(vx, vy), true, true);
    }
#endif

    // ================================================================
	// Blob
	// ================================================================

	ofWidth = ofGetWidth();
    ofHeight = ofGetHeight();

	camera.update();
	if(camera.isFrameNew()) {
	    camImg.setFromPixels(camera.getPixelsRef());
        camImg.mirror(false, true);
        blur(camImg, 10);
		contourFinder.findContours(camImg);
		tracker.track(contourFinder.getBoundingRects());
	}

	vector<Glow>& followers = tracker.getFollowers();
	for(int i = 0; i < followers.size(); i++) {
		//followers[i].draw();
		addToFluid(followers[i].getPos() / ofVec2f(camWidth, camHeight), followers[i].getVel() / ofVec2f(camWidth, camHeight), true, true);
	}

	fluidSolver.update();
}

void testApp::draw(){

    post.begin();

	if(drawFluid) {
        ofClear(0);
		glColor3f(1, 1, 1);
		fluidDrawer.draw(0, 0, ofGetWidth(), ofGetHeight());
	} else {
//		if(ofGetFrameNum()%5==0)
            fadeToColor(0, 0, 0, 0.01);
	}
	if(drawParticles)
		particleSystem.updateAndDraw(fluidSolver, ofGetWindowSize(), drawFluid);

//	ofDrawBitmapString(sz, 50, 50);

    // ================================================================
	// Blob
	// ================================================================

    ofPushMatrix();

    ofScale(ofWidth/(float(camWidth)), ofHeight/(float(camHeight)), 0.0);
	vector<Glow>& followers = tracker.getFollowers();
	for(int i = 0; i < followers.size(); i++) {
		followers[i].draw();
	}

    ofPopMatrix();

    post.end(false);
    post.draw(0, 0, ofWidth, ofHeight);

#ifdef USE_GUI
	gui.draw();
#endif
}

void testApp::keyPressed  (int key){
    switch(key) {
		case '1':
			fluidDrawer.setDrawMode(msa::fluid::kDrawColor);
			break;

		case '2':
			fluidDrawer.setDrawMode(msa::fluid::kDrawMotion);
			break;

		case '3':
			fluidDrawer.setDrawMode(msa::fluid::kDrawSpeed);
			break;

		case '4':
			fluidDrawer.setDrawMode(msa::fluid::kDrawVectors);
			break;

        case '5':
			post[0]->setEnabled(!post[0]->getEnabled());
			post[1]->setEnabled(!post[1]->getEnabled());
			post[2]->setEnabled(!post[2]->getEnabled());
			break;

        case '6':
			post[0]->setEnabled(!post[0]->getEnabled());
			post[3]->setEnabled(!post[3]->getEnabled());
			break;

        case '7':
			post[4]->setEnabled(!post[4]->getEnabled());
			break;

        case '8':
			post[5]->setEnabled(!post[5]->getEnabled());
			break;

		case 'd':
			drawFluid ^= true;
			break;

		case 'p':
			drawParticles ^= true;
			break;

		case 'f':
			ofToggleFullscreen();
			break;

		case 'R':
			fluidSolver.reset();
			break;

        case 's':
			camera.videoSettings();
			break;

		case 'b': {
//			Timer timer;
//			const int ITERS = 3000;
//			timer.start();
//			for(int i = 0; i < ITERS; ++i) fluidSolver.update();
//			timer.stop();
//			cout << ITERS << " iterations took " << timer.getSeconds() << " seconds." << std::endl;
		}
			break;

    }
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){
	ofVec2f eventPos = ofVec2f(x, y);
	ofVec2f mouseNorm = ofVec2f(eventPos) / ofGetWindowSize();
	ofVec2f mouseVel = ofVec2f(eventPos - pMouse) / ofGetWindowSize();
	addToFluid(mouseNorm, mouseVel, true, true);
	pMouse = eventPos;
}

void testApp::mouseDragged(int x, int y, int button) {
	ofVec2f eventPos = ofVec2f(x, y);
	ofVec2f mouseNorm = ofVec2f(eventPos) / ofGetWindowSize();
	ofVec2f mouseVel = ofVec2f(eventPos - pMouse) / ofGetWindowSize();
	addToFluid(mouseNorm, mouseVel, false, true);
	pMouse = eventPos;
}

