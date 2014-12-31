// =============================================================================
//
// Copyright (c) 2014 Seth Hunter <http://www.perspectum.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

//remove function bounding box
static bool shouldRemove(ofPtr<ofxBox2dBaseShape>shape) {
    return !ofRectangle(0, -400, ofGetScreenWidth(), ofGetScreenHeight()+400).inside(shape.get()->getPosition());
}

void ofApp::setup()
{
    //-------CAMERA SETUP------------------------------
    ofSetVerticalSync(false);
	camWidth = 640;
	camHeight = 480;
    camFrameRate = 120;

    //list devices - seems to only detect PS3 cameras
    std::vector<ofVideoDevice> devices = vidGrabber.listDevices();
    for(std::size_t i = 0; i < devices.size(); ++i)
    {
        std::stringstream ss;
        ss << devices[i].id << ": device name" << devices[i].deviceName;
        if(!devices[i].bAvailable)
        {
            ss << " - unavailable ";
        }
        ofLogNotice("USB camera id:") << ss.str();
	}

	vidGrabber.setDeviceID(0);
	vidGrabber.setDesiredFrameRate(camFrameRate);
	vidGrabber.setup(camWidth, camHeight);
    vidGrabber.setAutogain(false);
    vidGrabber.setAutoWhiteBalance(false);
    
    //-------HOMOGRAPHY SETUP ---------------------------
    fullScreen= false;
    movingPoint = false;
    saveMatrix = false;
    homographyReady = false;
    mirrorLeft = true;
    mirrorRight = true;
    showTracker = true;
    
    //TODO: make position of homography relative.
    sX = 0;
    sY = 200;
    ratio = 0.5;
    
    //set initial window states.
    ofSetWindowShape(camWidth*2, ofGetScreenHeight()-150);
    ofSetWindowPosition(ofGetScreenWidth()/2-camWidth, 50);
    if(fullScreen) ofSetFullscreen(true);
    
    videoPix.allocate(camWidth,camHeight,OF_PIXELS_RGBA);
    videoTexture.allocate(videoPix);
    videoImg.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
    warpedColor.allocate(camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
    
    // load the previous homography if it's available
    ofFile previous("homography.yml");
    if(previous.exists()) {
        FileStorage fs(ofToDataPath("homography.yml"), FileStorage::READ);
        fs["homography"] >> homography;
        homographyReady = true;
    }
    
    //load the previous draw points if available
    if(points.loadFile("points.xml")) {
        
        points.pushTag("leftPoints");
        int numP = points.getNumTags("p");
        for(int i=0; i<numP; i++) {
            points.pushTag("p",i);
            ofVec2f tempVec;
            tempVec.set(points.getValue("x",0),points.getValue("y",0));
            leftPoints.push_back(tempVec);
            points.popTag();
        }
        points.popTag();
        
        points.pushTag("rightPoints");
        numP = points.getNumTags("p");
        for(int i=0; i<numP; i++) {
            points.pushTag("p",i);
            ofVec2f tempVec;
            tempVec.set(points.getValue("x",0),points.getValue("y",0));
            rightPoints.push_back(tempVec);
            points.popTag();
        }
        points.popTag();
        
    }
    
    //------Box2D Setup ----------
    
    ofDisableAntiAliasing();
    ofSetLogLevel(OF_LOG_NOTICE);
    box2d.init();
    box2d.setGravity(0, 20);
    box2d.createGround();
    box2d.setFPS(30.0);
    
   
    //-------UI setup------------
    ofEnableSmoothing();
    ofSetCircleResolution(60);
    debugPos = ofPoint(10,camHeight+15);
    frameCount = 0;
    
    gui0 = new ofxUISuperCanvas("PS3 Camera");
    gui0->addSpacer();
    gui0->addMinimalSlider("EXPOSURE", 0, 255, 150.0);
    gui0->addMinimalSlider("BRIGHTNESS", 0, 255, 100.0);
    gui0->addMinimalSlider("GAIN", 0, 63, 50.0);
    gui0->addMinimalSlider("CONTRAST", 0, 255, 200.0);
    gui0->addMinimalSlider("SHARPNESS", 0, 255, 0.0);
    gui0->addLabelButton("SAVE SETTINGS", false);
    gui0->autoSizeToFitWidgets();
    ofAddListener(gui0->newGUIEvent,this,&ofApp::guiEvent);
    
    
    gui1 = new ofxUISuperCanvas("Homography");
    gui1->addSpacer();
    gui1->addToggle("  MIRROR FULLSCREEN", true);
    gui1->addToggle("  TOGGLE FULLSCREEN", false);
    gui1->addToggle("  LOCK/UNLOCK POINTS", false); 
    gui1->addLabelButton("CLEAR HOMOGRAPHY", false);
    gui1->addLabelButton("SAVE HOMOGRAPHY", false);
    gui1->autoSizeToFitWidgets();
    ofAddListener(gui1->newGUIEvent,this,&ofApp::guiEvent);
    
    
    gui2 = new ofxUISuperCanvas("Tracking");
    gui2->addSpacer();
    gui2->addToggle("SHOW/HIDE TRACKING", true);
    gui2->addToggle("INVERT TRACKING", true);
    gui2->addMinimalSlider("THRESHOLD", 0, 255, 128.0);
    gui2->addMinimalSlider("MIN AREA RADIUS", 0, 200, 15.0);
    gui2->addMinimalSlider("MAX AREA RADIUS", 0, 200, 100.0);
    gui2->addMinimalSlider("PERSISTENCE", 0, 60, 15.0);
    gui2->addMinimalSlider("MAX DISTANCE", 0, 250, 32.0);
    gui2->addLabelButton("SAVE TRACKING", false);
    gui2->autoSizeToFitWidgets();
    ofAddListener(gui2->newGUIEvent,this,&ofApp::guiEvent);  //load settings triggers event updates
    
    
}


void ofApp::updatePostions() {
    if(fullScreen) {
        gui0->setPosition(0,ofGetWindowHeight()-175);
        gui1->setPosition(ofGetWindowWidth()-gui1->getGlobalCanvasWidth(),ofGetWindowHeight()-200);
        gui2->setPosition(ofGetWindowWidth()-gui2->getGlobalCanvasWidth()-gui1->getGlobalCanvasWidth()-25,ofGetWindowHeight()-200);
    } else {
        gui0->setPosition(gui0->getGlobalCanvasWidth()+camWidth+190,camHeight);
        gui1->setPosition(camWidth-60, camHeight);
        gui2->setPosition(gui2->getGlobalCanvasWidth()+camWidth-50,camHeight);
    }
}


void ofApp::update()
{
	//-----------------PS3--------------------------
    updatePostions();
    vidGrabber.update();
	if (vidGrabber.isFrameNew())
    {
		videoTexture.loadData(vidGrabber.getPixels());
        videoPix.setFromPixels(vidGrabber.getPixels(), camWidth, camHeight, OF_PIXELS_RGBA);
	}
    
    //-----------------video homography---------------------
    if(fullScreen) lockHomography = true;
    
    if(!lockHomography) {
        if(leftPoints.size() >= 4) {
            vector<Point2f> srcPoints, dstPoints;
            for(int i = 0; i < leftPoints.size(); i++) {
                srcPoints.push_back(Point2f(rightPoints[i].x - camWidth, rightPoints[i].y));
                dstPoints.push_back(Point2f(leftPoints[i].x, leftPoints[i].y));
            }
            
            // generate a homography from the two sets of points
            homography = findHomography(Mat(srcPoints), Mat(dstPoints));
            homographyReady = true;
            
            if(saveMatrix) {
                FileStorage fs(ofToDataPath("homography.yml"), FileStorage::WRITE);
                fs << "homography" << homography;
                saveMatrix = false;
            }
        }
    }
    
    if(homographyReady) {
        // this is how you warp one ofImage into another ofImage given the homography matrix
        // CV INTER NN is 113 fps, CV_INTER_LINEAR is 93 fps
        //warpPerspective(right, warpedColor, homography, CV_INTER_LINEAR);
        warpPerspective(videoPix, warpedColor, homography, CV_INTER_LINEAR);
        warpedColor.update();
        
        if(fullScreen) {
            if(mirrorLeft)warpedColor.mirror(false, true);
        }
    }
    
    //-----------------tracking--------------------------
    
    blur(warpedColor,5);
    contourFinder.findContours(warpedColor);
    
    //having some strange NaN behaviors while initializing
    frameCount++;
    if((frameCount == 30) || (frameCount==60)){
        gui0->loadSettings("PS3_Settings.xml");
        gui1->loadSettings("Homography_Settings.xml");
        gui2->loadSettings("Tracking_Settings.xml");
    }
    
    //-----------------Box2D --------------------
    
    if(fullScreen) {
        // add some circles every so often
        if((int)ofRandom(0, 10) == 0) {
            shared_ptr<ofxBox2dCircle> circle = shared_ptr<ofxBox2dCircle>(new ofxBox2dCircle);
            circle.get()->setPhysics(0.3, 0.5, 0.1);
            circle.get()->setup(box2d.getWorld(), (ofGetWidth()/2)+ofRandom(-20, 20), -20, ofRandom(10, 20));
            circles.push_back(circle);
        }
        
        // remove shapes offscreen
        ofRemove(circles, shouldRemove);
        ofRemove(polyShapes, shouldRemove);
        
        polyShapes.clear();
        //RectTracker& tracker = contourFinder.getTracker();   //use to get labels
        for(int i = 0; i < contourFinder.size(); i++) {
            ofPolyline temp = contourFinder.getPolyline(i);
            createBox2DShape(temp);
        }
        
        box2d.update();
    }
    
}

void ofApp::draw()
{
    std::stringstream ss;
    ofBackground(45);
    ofSetColor(255);
    ofSetFrameRate(120);
    
    ofShowCursor();
    
    if(!fullScreen) {
        
        ss << "App FPS: " << ofGetFrameRate() << std::endl;
        ss << "Cam FPS: " << vidGrabber.getFPS() << std::endl;
        ss << "mode: draw homography" << std::endl;
        videoTexture.draw(camWidth, 0, camWidth, camHeight);
        if(homographyReady) {
            warpedColor.draw(0, 0);
        } else {
            videoTexture.draw(0,0);
        }
        
        //drawing lines
        ofSetColor(ofColor::red);
        drawPoints(leftPoints);
        ofSetColor(ofColor::blue);
        drawPoints(rightPoints);
        ofSetColor(128);
        for(int i = 0; i < leftPoints.size(); i++) {
            ofDrawLine(leftPoints[i], rightPoints[i]);
        }
        
        ofSetColor(255);
        ofDrawBitmapString(ofToString((int) ofGetFrameRate()), 10, 20);
        //draw debug top left
        ss << "\nkeyboard shortcuts:\n s=save-homography \n f=toggle-fullscreen \n g=toggle-GUIs" << std::endl;
        ofDrawBitmapStringHighlight(ss.str(), debugPos);
        
        std::stringstream dir;
        dir << "Directions:" << std::endl;
        dir << "1) Use the PS3 Camera GUI to adjust your \n video image. Click 'save settings'." << std::endl;
        dir << "2) Click 4 points on the right image to \n mark the corners of your screen." << std::endl;
        dir << "3) Adjust the red points on the left by \n clicking and moving." << std::endl;
        dir << "4) Click 'Save Homography' to save to file." << std::endl;
        dir << "5) Toggle Fullscreen to perform. \n Try keyboard shortcuts (see left legend)." << std::endl;
        ofDrawBitmapStringHighlight(dir.str(), debugPos + ofPoint(200,0));
        
        if(showTracker) drawTracker();
        
    }
    else {
        ss << "mode: fullscreen" << std::endl;
        warpedColor.draw(0, 0,ofGetWindowWidth(), ofGetWindowHeight());
        
        //------box2D stuff-------------------
        // circles
        for (int i=0; i<circles.size(); i++) {
            ofFill();
            ofSetHexColor(0xc0dd3b);
            circles[i].get()->draw();
        }
        //polyshapes
        ofSetHexColor(0x444342);
        ofNoFill();
        for (int i=0; i<polyShapes.size(); i++) {
            polyShapes[i].get()->draw();
            ofDrawCircle(polyShapes[i].get()->getPosition(), 3);
        }
        
        
    }
    
}

void ofApp::drawTracker() {
    
    RectTracker& tracker = contourFinder.getTracker();
    ofSetColor(255);
    contourFinder.draw();
    
    for(int i = 0; i < contourFinder.size(); i++) {
        ofPoint center = toOf(contourFinder.getCenter(i));
        ofPushMatrix();
        ofTranslate(center.x, center.y);
        int label = contourFinder.getLabel(i);
        string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
        ofDrawBitmapString(msg, 0, 0);
        ofVec2f velocity = toOf(contourFinder.getVelocity(i));
        ofScale(5, 5);
        ofDrawLine(0, 0, velocity.x, velocity.y);
        ofPopMatrix();
    }

}

void ofApp::drawPoints(vector<ofVec2f>& points) {
    ofNoFill();
    for(int i = 0; i < points.size(); i++) {
        ofDrawCircle(points[i], 10);
        ofDrawCircle(points[i], 1);
    }
}

//converts a contour into a box 2D shape
void ofApp::createBox2DShape(ofPolyline &daShape) {
    
    
    daShape = daShape.getResampledByCount(b2_maxPolygonVertices);
    //daShape = getConvexHull(shape); //we don't need this because contourfinder returns a convex hull
    shared_ptr<ofxBox2dPolygon> poly = shared_ptr<ofxBox2dPolygon>(new ofxBox2dPolygon);
    poly.get()->addVertices(daShape.getVertices());
    poly.get()->setPhysics(1.0, 0.3, 0.3);
    poly.get()->create(box2d.getWorld());
    polyShapes.push_back(poly);
    daShape.clear();
}



//--------------------------------------------------------------
void ofApp::guiEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    int kind = e.widget->getKind();
    
    //homography
    if(name == "CLEAR HOMOGRAPHY") {
        clearPoints();
    }
    else if (name == "SAVE HOMOGRAPHY") {
        saveMatrix = true;
        gui1->saveSettings("Homography_Settings.xml");
    }
    else if (name == "  MIRROR FULLSCREEN") {
        ofxUIToggle *temp = (ofxUIToggle *) e.widget;
        bool mirror = temp->getValue();
        mirrorLeft = mirror;
    }
    else if (name == "  TOGGLE FULLSCREEN") {
        ofxUIToggle *temp = (ofxUIToggle *) e.widget;
        fullScreen = temp->getValue();
        ofSetFullscreen(fullScreen);
    }
    else if (name == "  LOCK/UNLOCK POINTS") {
        ofxUIToggle *temp = (ofxUIToggle *) e.widget;
        lockHomography = temp->getValue();
    }
    //camera settings
    else if (name == "EXPOSURE") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        vidGrabber.setExposure((uint8_t)holdme);
    }
    else if (name == "BRIGHTNESS") {
        ofxUISlider *exp = (ofxUISlider *) e.widget;
        float holdme = exp->getValue();
        vidGrabber.setBrightness((uint8_t)holdme);
    }
    else if (name == "SHARPNESS") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        vidGrabber.setSharpness((uint8_t)holdme);
    }
    else if (name == "CONTRAST") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        vidGrabber.setContrast((uint8_t)holdme);
    }
    else if (name == "GAIN") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        vidGrabber.setGain((uint8_t)holdme);
    }
    else if (name == "SAVE SETTINGS") {
        gui0->saveSettings("PS3_Settings.xml");
    }
    //tracking
    else if (name == "SHOW/HIDE TRACKING") {
        ofxUIToggle *temp = (ofxUIToggle *) e.widget;
        showTracker = temp->getValue();
    }
    else if (name == "INVERT TRACKING") {
        ofxUIToggle *temp = (ofxUIToggle *) e.widget;
        contourFinder.setInvert(temp->getValue());
    }
    else if (name == "THRESHOLD") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        contourFinder.setThreshold(holdme);
    }
    else if (name == "MIN AREA RADIUS") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        contourFinder.setMinAreaRadius(holdme);
    }
    else if (name == "MAX AREA RADIUS") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        contourFinder.setMaxAreaRadius(holdme);
    }
    else if (name == "PERSISTENCE") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        contourFinder.getTracker().setPersistence(holdme);
    }
    else if (name == "MAX DISTANCE") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        float holdme = temp->getValue();
        contourFinder.getTracker().setMaximumDistance(holdme);
    }
    else if (name == "SAVE TRACKING") {
        gui2->saveSettings("Tracking_Settings.xml");
    }
    
}

//--------------------------------------------------------------
void ofApp::exit()
{
    delete gui0;
}

void ofApp::clearPoints() {
    leftPoints.clear();
    rightPoints.clear();
    points.clear();
    points.saveFile("points.xml");
    homographyReady = false;
    
    ofFile previous("homography.yml");
    if(previous.exists()) {
        previous.remove();
    }
}

bool ofApp::movePoint(vector<ofVec2f>& points, ofVec2f point, int LeftOrRight) {
    for(int i = 0; i < points.size(); i++) {
        if(points[i].distance(point) < 20) {
            movingPoint = true;
            curPoint = &points[i];
            curPointIndex = i;
            curPointLeftOrRight = LeftOrRight;
            return true;
        }
    }
    return false;
}

void ofApp::mousePressed(int x, int y, int button) {
    if(!lockHomography) {
        if((x < camWidth*2) && (y<camHeight)) {
            ofVec2f cur(x, y);
            ofVec2f rightOffset(camWidth, 0);
            if(!movePoint(leftPoints, cur, 0) && !movePoint(rightPoints, cur, 1)) {
                if(x > camWidth) {
                    cur -= rightOffset;
                }
                leftPoints.push_back(cur);
                rightPoints.push_back(cur + rightOffset);
                saveXMLPoints(cur);
            }
        }
    }
}

//pushes a single position change into the XML representaiton and file.
void ofApp::pushXMLPoint(ofVec2f point, int index, int LorR) {
    if(LorR==0) points.pushTag("leftPoints");
    else if(LorR==1) points.pushTag("rightPoints");
    points.pushTag("p",index);
    points.setValue("x", point.x);
    points.setValue("y", point.y);
    points.popTag();
    points.popTag();
    points.saveFile("points.xml");
}

//creates the initial XML structure for points on click
void ofApp::saveXMLPoints(ofVec2f cur) {
    
    ofVec2f rightOffset(camWidth, 0);
    //left
    if(!points.pushTag("leftPoints")) {
        points.addTag("leftPoints");
        points.pushTag("leftPoints");
    }
    points.addTag("p");
    points.pushTag("p",leftPoints.size()-1);
    points.addValue("x", cur.x);
    points.addValue("y", cur.y);
    points.popTag();
    points.popTag();
    //right
    if(!points.pushTag("rightPoints")) {
        points.addTag("rightPoints");
        points.pushTag("rightPoints");
    }
    points.addTag("p");
    points.pushTag("p",rightPoints.size()-1);
    points.addValue("x", cur.x + rightOffset.x);
    points.addValue("y", cur.y + rightOffset.y);
    points.popTag();
    points.popTag();
    points.saveFile("points.xml");
    
}



//updates the value of the current point.
void ofApp::mouseDragged(int x, int y, int button) {
    if(movingPoint && !lockHomography) {
        curPoint->set(x, y);
    }
}

void ofApp::mouseReleased(int x, int y, int button) {
    //pushes released point into XML file
    if(movingPoint && !lockHomography ) pushXMLPoint(ofVec2f(x,y),curPointIndex,curPointLeftOrRight);
    movingPoint = false;
}

void ofApp::keyPressed(int key) {
    //save matrix settings
    if(key == 'g') {
        gui0->toggleVisible();
        gui1->toggleVisible();
        gui2->toggleVisible();
    }
    //save homography
    else if(key == 's') {
        saveMatrix = true;
    }
    //toggle fullscreen
    else if(key == 'f') {
        fullScreen = !fullScreen;
        ofToggleFullscreen();
        
    }
    //box2D clear
    else if(key == 'c') {
        shape.clear();
        polyShapes.clear();
        circles.clear();
    }
    //box2d create circles
    if(key == '1') {
        shared_ptr<ofxBox2dCircle> circle = shared_ptr<ofxBox2dCircle>(new ofxBox2dCircle);
        circle.get()->setPhysics(0.3, 0.5, 0.1);
        circle.get()->setup(box2d.getWorld(), mouseX, mouseY, ofRandom(10, 20));
        circles.push_back(circle);
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y )
{

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
    
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}
