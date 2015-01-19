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

//CALLING THIS TOO FREQUENTLY WILL SLOW FRAMERATE
static bool shouldRemove(ofPtr<ofxBox2dBaseShape>shape) {
    return !ofRectangle(0, -400, ofGetWidth(), ofGetHeight()+400).inside(shape.get()->getPosition());
}

void ofApp::setup()
{
    //-------CAMERA SETUP------------------------------
    ofSetVerticalSync(false);
	camWidth = 320;
	camHeight = 240;
    camFrameRate = 120;
    xOffset = 1440;
    displayWidth = xOffset;
    projectorWidth = 1024;
    projectorHeight = 768; 

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
    mirrorLeft = false;
    mirrorRight = true;
    showTracker = true;
    
    markProjectorBounds = false;
    drawProjectorBounds = true;
    applyProjectorHomography = false;
    
    sX = 0;
    sY = 200;
    ratio = 0.5;
    
    //set initial window states.
   // ofSetWindowShape(camWidth*2+60, camHeight*3);
    ofSetWindowPosition(0, 0);
    if(fullScreen) ofSetFullscreen(true);
    
    videoPix.allocate(camWidth,camHeight,OF_PIXELS_RGBA);
    videoTexture.allocate(videoPix);
    videoImg.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
    warpedColor.allocate(camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
    projectorWarp.allocate(camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
    
    // load the previous homography if it's available
    ofFile previous("homography.yml");
    if(previous.exists()) {
        FileStorage fs(ofToDataPath("homography.yml"), FileStorage::READ);
        fs["homography"] >> homography;
        homographyReady = true;
    }
    
    //load the previous camera homography points
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
    
    //load the marked corners of the projection.
    if(projXML.loadFile("projectorPoints.xml")) {
        projXML.pushTag("Points");
        int numP = projXML.getNumTags("p");
        for(int i=0; i<numP; i++) {
            projXML.pushTag("p",i);
            ofVec2f tempVec;
            tempVec.set(projXML.getValue("x",0),projXML.getValue("y",0));
            projectorPoints.push_back(tempVec);
            projXML.popTag();
        }
        projXML.popTag();
    }
    
    if(projectorPoints.size() >= 4) {
        writeProjectorPoints();
    }
    
    
    //------Box2D Setup ----------
    
    ofDisableAntiAliasing();
    ofSetLogLevel(OF_LOG_NOTICE);
    box2d.init();
    box2d.setGravity(20.0, 0.0);
    box2d.createGround();
    box2d.setFPS(30.0);
    addWalls();
   
    //-------UI setup------------
    ofEnableSmoothing();
    ofSetCircleResolution(60);
    debugPos = ofPoint(10,camHeight+35);
    frameCount = 0;
    
    //TODO: - pull the develop branch of ofxUI to fix this issue
    //https://github.com/rezaali/ofxUI/issues/218  discusses the initialization issue.
    
    gui0 = new ofxUISuperCanvas("PS3 Camera");
    gui0->addSpacer();
    gui0->addMinimalSlider("EXPOSURE", 0.0, 255.0, 150.0);
    gui0->addMinimalSlider("BRIGHTNESS", 0.0, 255.0, 100.0);
    gui0->addMinimalSlider("GAIN", 0.0, 63.0, 50.0);
    gui0->addMinimalSlider("CONTRAST", 0.0, 255.0, 200.0);
    gui0->addMinimalSlider("SHARPNESS", 0.0, 255.0, 0.0);
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
    gui1->addLabelButton("REFRESH GUIS", false);
    gui1->autoSizeToFitWidgets();
    ofAddListener(gui1->newGUIEvent,this,&ofApp::guiEvent);
    
    gui2 = new ofxUISuperCanvas("Tracking");
    gui2->addSpacer();
    gui2->addToggle("SHOW/HIDE TRACKING", true);
    gui2->addToggle("INVERT TRACKING", true);
    gui2->addMinimalSlider("THRESHOLD", 0.0, 255.0, 128.0);
    gui2->addMinimalSlider("MIN AREA RADIUS", 0.0, 200.0, 15.0);
    gui2->addMinimalSlider("MAX AREA RADIUS", 0.0, 200.0, 100.0);
    gui2->addMinimalSlider("PERSISTENCE", 0.0, 60.0, 15.0);
    gui2->addMinimalSlider("MAX DISTANCE", 0.0, 250.0, 32.0);
    gui2->addLabelButton("SAVE TRACKING", false);
    gui2->autoSizeToFitWidgets();
    ofAddListener(gui2->newGUIEvent,this,&ofApp::guiEvent);  //load settings triggers event updates
    
    gui3 = new ofxUISuperCanvas("Box2D");
    gui3->addSpacer();
    gui3->addToggle("GRAVITY ON", true);
    gui3->addToggle("WALLS ON", true);
    gui3->addMinimalSlider("CIRCLE MIN", 0.0, 200.0, 2.0);
    gui3->addMinimalSlider("CIRCLE MAX", 0.0, 200.0, 20.0);
    gui3->addMinimalSlider("CIRCLE FREQ", 0.0, 50.0, 20.0);
    gui3->addLabelButton("ADD CIRCLE", false);
    gui3->addLabelButton("ADD PARTICLES", false);
    gui3->addLabelButton("CLEAR SHAPES", false);
    gui3->addLabelButton("SAVE BOX2D", false);
    gui3->autoSizeToFitWidgets();
    ofAddListener(gui3->newGUIEvent,this,&ofApp::guiEvent);  //load settings triggers event updates

    refreshGUIs();
    
}

void ofApp::addWalls() {
    //add walls
    float wallSize = 30;
    float buffer = 5;
    //top
    //TODO: fix walls - they are not sized correctly
    walls.push_back(shared_ptr<ofxBox2dRect>(new ofxBox2dRect));
    walls.back().get()->setPhysics(0.0, 0.0, 0.0);
    walls.back().get()->setup(box2d.getWorld(), displayWidth-wallSize, -1*wallSize+buffer, projectorWidth + wallSize*2, wallSize);
    //right
    walls.push_back(shared_ptr<ofxBox2dRect>(new ofxBox2dRect));
    walls.back().get()->setPhysics(0.0, 0.0, 0.0);
    walls.back().get()->setup(box2d.getWorld(), displayWidth+projectorWidth, -1*wallSize+buffer, wallSize, projectorHeight + wallSize*2);
    //left
    walls.push_back(shared_ptr<ofxBox2dRect>(new ofxBox2dRect));
    walls.back().get()->setPhysics(0.0, 0.0, 0.0);
    walls.back().get()->setup(box2d.getWorld(), displayWidth-wallSize+buffer, -1*wallSize+buffer, wallSize, projectorHeight + wallSize*2);

}

void ofApp::updateGUIPostions() {
    float guiPos = camHeight*2.0;
        gui0->setPosition(450,guiPos);
        gui1->setPosition(0, guiPos);
        gui2->setPosition(210,guiPos);
        gui3->setPosition(680,guiPos);
}


void ofApp::update()
{
	//-----------------PS3--------------------------
    updateGUIPostions();
    vidGrabber.update();
	if (vidGrabber.isFrameNew())
    {
		videoTexture.loadData(vidGrabber.getPixels());
        videoPix.setFromPixels(vidGrabber.getPixels(), camWidth, camHeight, OF_PIXELS_RGBA);
	}
    
    //-----------------video homography---------------------
   // if(fullScreen) lockHomography = true;
    
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
        
        if(applyProjectorHomography) {
            warpPerspective(warpedColor, projectorWarp, projectorHomography, CV_INTER_LINEAR);
            projectorWarp.update();
        }
        
       // if(fullScreen) {
            if(mirrorLeft)warpedColor.mirror(false, true);
       // }
    }
    
    //-----------------tracking--------------------------
    
    blur(warpedColor,5);
    contourFinder.findContours(warpedColor);
    
    //having some strange NaN behaviors while initializing
    frameCount++;
    if(frameCount == 30){
        refreshGUIs();
    }
    
    //-----------------Box2D --------------------
    
        // add some circles every so often
    /*
        if((int)ofRandom(0, circleFreq) == 0) {
            shared_ptr<ofxBox2dCircle> circle = shared_ptr<ofxBox2dCircle>(new ofxBox2dCircle);
            circle.get()->setPhysics(0.3, 0.5, 0.1);
            circle.get()->setup(box2d.getWorld(), xOffset + ((ofGetWidth()-xOffset)/2) + ofRandom(-20, 20), -20, ofRandom(circleMin, circleMax));
            circles.push_back(circle);
        }
     */
        
        // remove shapes offscreen
        ofRemove(circles, shouldRemove);
        ofRemove(customParticles, shouldRemove);
    
        polyShapes.clear();
        //RectTracker& tracker = contourFinder.getTracker();   //use to get labels
        for(int i = 0; i < contourFinder.size(); i++) {
            ofPolyline temp = contourFinder.getPolyline(i);
            createBox2DShape(temp);
            updateBox2DForces(contourFinder.getCentroid(i));
        }
    
        box2d.update();
    
}

void ofApp::updateBox2DForces(cv::Point2f centroid) {
    float strength = 8.0f;
    float damping  = 0.7f;
    float minDis   = 100;
    float ratioW = projectorWidth/camWidth;
    float ratioH = projectorHeight/camHeight;
    for(int i=0; i<circles.size(); i++) {
        if(drawProjectorBounds) {
        circles[i].get()->addAttractionPoint(centroid.x*ratioW + projectorPoints[0].x, centroid.y*ratioH + projectorPoints[0].y, strength);
        circles[i].get()->setDamping(damping, damping);
        }
    }
    for(int i=0; i<customParticles.size(); i++) {
        if(drawProjectorBounds) {
        customParticles[i].get()->addAttractionPoint(centroid.x*ratioW + projectorPoints[0].x, centroid.y*ratioH + projectorPoints[0].y, strength);
        customParticles[i].get()->setDamping(damping, damping);
        }
    }
}


void ofApp::refreshGUIs(){
    gui0->loadSettings("PS3_Settings.xml");
    gui1->loadSettings("Homography_Settings.xml");
    gui2->loadSettings("Tracking_Settings.xml");
    gui3->loadSettings("Box2d_Settings.xml");
}


void ofApp::draw()
{
    std::stringstream dir;
    ofBackground(0);
    ofSetColor(255);
    ofSetFrameRate(120);
    ofShowCursor();
    
    dir << "App FPS: " << ofGetFrameRate() << std::endl;
    dir << "Cam FPS: " << vidGrabber.getFPS() << std::endl;
    
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
    
    dir << "Total Bodies: " << ofToString(box2d.getBodyCount()) << "\n";
    dir << "Total Joints: " << ofToString(box2d.getJointCount()) << "\n\n";
    
    dir << "Directions:" << std::endl;
    dir << "1) Use the PS3 Camera GUI to adjust your video image. Click 'save settings'." << std::endl;
    dir << "2) Click 4 points on the right image to mark the corners of your screen." << std::endl;
    dir << "3) Adjust the red points on the left by clicking and moving." << std::endl;
    dir << "4) Click 'Save Homography' to save to file." << std::endl;
    dir << "5) Press 'p' to mark the 4 corners of your projection area (top-left,top-right,bot-right,bot-left)" << std::endl;
    dir << "6) Use the control panels to adjust tracking parameters, and add physics." << std::endl;

    if(showTracker) drawTracker();

    
    //after clicking 4 points in p mode this image should appear corrected.
    //if(applyProjectorHomography) projectorWarp.draw(xOffset, 0, projectorWidth, projectorHeight);
    //else warpedColor.draw(xOffset, 0, 1024, 768);

    //------box2D stuff-------------------
    
    //walls
    for (int i=0; i<walls.size(); i++) {
        ofFill();
        ofSetHexColor(0xc0dd3b);
        walls[i].get()->draw();
    }
    
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
    //particles
    for(int i=0; i<customParticles.size(); i++) {
        customParticles[i].get()->draw();
    }
    

    drawProjectorRect(); 
    
    //ofSetColor(0, 0, 0);
    ofDrawBitmapStringHighlight(dir.str(), debugPos + ofPoint(0,0));
    
}


void ofApp::drawProjectorRect() {
    if(drawProjectorBounds) {
        ofPolyline projShape;
        projShape.addVertices(projectorPoints);
        ofSetColor(0, 0, 255);
        projShape.draw();
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
    poly.get()->addVertices(scalePolyShape(daShape));
    poly.get()->setPhysics(1.0, 0.3, 0.3);
    poly.get()->create(box2d.getWorld());
    polyShapes.push_back(poly);
    daShape.clear();
}

//helper function to scale points in fullscreen mode
vector<ofPoint> ofApp::scalePolyShape(ofPolyline shapeIn) {
    float xScale, yScale; 
    xScale = ofGetScreenWidth()/camWidth;
    yScale = ofGetScreenHeight()/camHeight;
    vector<ofPoint> pts = shapeIn.getVertices();
    
    //THEORY:
    //use the homography to translate the points from 0,320 to the new homography coordinates.
    //then scale the points to the projector size
    //then translate the points by the projectorPoints[0] point the top left origin
    if(applyProjectorHomography) {
        xScale = projectorWidth/camWidth;
        yScale = projectorHeight/camHeight;
        //translate point values between homography
        std::vector<Point2f> ptsIn(pts.size());
        std::vector<Point2f> ptsOut(pts.size());
        for(int i=0; i<pts.size(); i++) {
            ptsIn[i] = Point2f(pts[i].x,pts[i].y);
        }
        cv::perspectiveTransform(ptsIn, ptsOut, projectorHomography);
        for(int i=0; i<pts.size(); i++) {
            //copy points back in
            pts[i].x = ptsOut[i].x;
            pts[i].y = ptsOut[i].y;
            //scale points to projector
            pts[i].x *= xScale;
            pts[i].y *= yScale;
            //shift points by projector offset
            pts[i].x += displayWidth;
        }
        
    } else {
        for(int i=0; i<pts.size(); i++) {
            pts[i].x *= xScale;
            pts[i].y *= yScale;
        }
    }
    return pts;
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
    else if(name == "REFRESH GUIS") {
        refreshGUIs();
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
    
    //------------------BOX 2D MODDING -------------------//
    
    else if (name == "WALLS ON") {
        ofxUIToggle *temp = (ofxUIToggle *) e.widget;
        wallsOn = temp->getValue();
        if(wallsOn) {
            addWalls();
        } else {
            walls.clear();
        }
    }
    else if (name == "GRAVITY ON") {
        ofxUIToggle *temp = (ofxUIToggle *) e.widget;
        gravityOn = temp->getValue();
        if(gravityOn) {
            box2d.setGravity(20.0, 0.0);
        } else {
            box2d.setGravity(0.0, 0.0);
        }
    }
    else if (name == "ADD CIRCLE") {
        float r = ofRandom(4, 20);
        float x = ofRandom(displayWidth, displayWidth+projectorWidth);
        float y = ofRandom(0, -100);
        circles.push_back(shared_ptr<ofxBox2dCircle>(new ofxBox2dCircle));
        circles.back().get()->setPhysics(3.0, 0.53, 0.1);
        circles.back().get()->setup(box2d.getWorld(), x, y, r);
    }
    else if (name == "CIRCLE MIN") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        circleMin = temp->getValue();
    }
    else if (name == "CIRCLE MAX") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        circleMax = temp->getValue();
    }
    else if (name == "CIRCLE FREQ") {
        ofxUISlider *temp = (ofxUISlider *) e.widget;
        circleFreq = temp->getValue();
    }
    else if (name == "ADD PARTICLES") {
        customParticles.push_back(shared_ptr<CustomParticle>(new CustomParticle));
        CustomParticle * p = customParticles.back().get();
        float r = ofRandom(3, 20);
        float x = ofRandom(displayWidth, displayWidth+projectorWidth);
        float y = ofRandom(0, -100);
        p->setPhysics(0.4, 0.53, 0.31);
        p->setup(box2d.getWorld(), x, y, r);
        p->color.r = ofRandom(20, 100);
        p->color.g = 0;
        p->color.b = ofRandom(150, 255);
    }
    else if (name == "CLEAR SHAPES") {
        circles.clear();
        customParticles.clear();
    }
    else if (name == "SAVE BOX2D") {
        gui3->saveSettings("Box2d_Settings.xml");
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
    if(!markProjectorBounds) {
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
    } else {
        //each click adds a point to the vector
        projectorPoints.push_back(ofPoint(x,y));
        writeProjectorPoints();
    }
}


void ofApp::writeProjectorPoints() {
    if(projectorPoints.size() ==4) {
        
        //reset the box2d world.
        box2d.createGround(ofPoint(projectorPoints[3].x,projectorPoints[3].y), ofPoint(projectorPoints[2].x, projectorPoints[2].y));
        
        //find the projector homography once
        float ratio = camWidth/projectorWidth;
        vector<Point2f> srcPoints;
        vector<Point2f> dstPoints;
        for(int i=0; i<projectorPoints.size(); i++){
            //shift and scale points
            dstPoints.push_back(Point2f((projectorPoints[i].x-displayWidth)*ratio, projectorPoints[i].y*ratio));
            if(markProjectorBounds) saveProjectorPoint(ofVec2f(projectorPoints[i].x, projectorPoints[i].y), i);
        }
        
        srcPoints.push_back(Point2f(0.0, 0.0));
        srcPoints.push_back(Point2f((float)camWidth, 0.0));
        srcPoints.push_back(Point2f((float)camWidth, (float)camHeight));
        srcPoints.push_back(Point2f(0.0, (float)camHeight));
        projectorHomography = findHomography(Mat(srcPoints), Mat(dstPoints));
        applyProjectorHomography = true;   //might be nice to make sure it calculated correctly.
        
        //add a 5th point to draw the complete outline.
        ofPoint first = projectorPoints[0];
        projectorPoints.push_back(first);
        
        drawProjectorBounds=true;
        markProjectorBounds=false;
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

//saves the global coordinates of the projector points for next time
void ofApp::saveProjectorPoint(ofVec2f cur, int index) {
    
    //left
    if(!projXML.pushTag("Points")) {
        projXML.addTag("Points");
        projXML.pushTag("Points");
    }
    if(!projXML.pushTag("p",index)) {
        projXML.addTag("p");
        projXML.pushTag("p",index);
        projXML.addValue("x", cur.x);
        projXML.addValue("y", cur.y);
    } else {
        projXML.pushTag("p",index);
        projXML.setValue("x", cur.x);
        projXML.setValue("y", cur.y);
    }
    
    projXML.popTag();
    projXML.popTag();
    projXML.saveFile("projectorPoints.xml");

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
        //fullScreen = !fullScreen;
        //ofToggleFullscreen();
    }
    //box2D clear
    else if(key == 'c') {
        shape.clear();
        polyShapes.clear();
        circles.clear();
    }
    //box2d create circles
    else if(key == '1') {
        shared_ptr<ofxBox2dCircle> circle = shared_ptr<ofxBox2dCircle>(new ofxBox2dCircle);
        circle.get()->setPhysics(0.3, 0.5, 0.1);
        circle.get()->setup(box2d.getWorld(), mouseX, mouseY, ofRandom(10, 20));
        circles.push_back(circle);
    }
    else if(key == 'p') {
        markProjectorBounds = true;
        applyProjectorHomography = false;
        projectorPoints.clear();
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
