// =============================================================================
//
// Copyright (c) 2014 Seth Hunter
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


#pragma once


#include "ofMain.h"
#include "ofxPS3EyeGrabber.h"
#include "ofxCv.h"
#include "ofxUI.h"
#include "ofxBox2d.h"
#include "customParticle.h"

class ofApp: public ofBaseApp
{
public:
    
    //-------standard
    void setup();
    void update();
    void draw();
    void exit();
    void mousePressed(int x, int y, int button);
    void mouseDragged(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void keyPressed(int key);
    
    //---------homography
    bool movePoint(vector<ofVec2f>& points, ofVec2f point, int LeftOrRight);
    void drawPoints(vector<ofVec2f>& points);
    void updateGUIPostions();
    void clearPoints();
    void saveXMLPoints(ofVec2f cur);
    void pushXMLPoint(ofVec2f point, int index, int LorR);
    
    //--------- ofxUI
    void mouseMoved(int x, int y );
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    ofxUISuperCanvas *gui0; 
    ofxUISuperCanvas *gui1;
    ofxUISuperCanvas *gui2;
    ofxUISuperCanvas *gui3; 
    void guiEvent(ofxUIEventArgs &e);
    int frameCount;
    void refreshGUIs(); 
    
    //---------General Parameters
    bool                fullScreen;
    
    //----------PS3 Camera Control
    ofxPS3EyeGrabber vidGrabber;
    //ofTexture videoTexture;
    int camWidth;
    int camHeight;
    int camFrameRate;
    ofPixels		 	videoPix;
    ofImage             videoImg;
    ofTexture			videoTexture;
    ofTexture           warpedTexture;
    
    //------------Homography
    float sX, sY, ratio;
    ofPoint debugPos; 
    ofImage warpedColor;
    vector<ofVec2f> leftPoints, rightPoints;
    bool movingPoint, mirrorLeft, mirrorRight;
    ofVec2f* curPoint;
    int curPointIndex, curPointLeftOrRight;
    bool saveMatrix;
    bool homographyReady, lockHomography;
    ofxXmlSettings points;
    ofxXmlSettings projXML; 
    cv::Mat homography;
    
    //------------Tracking
    void drawTracker(); 
    ofxCv::ContourFinder contourFinder;
    float threshold;
    bool showTracker;
    
    //-------------Projector Space
    float projectorWidth, projectorHeight;
    float displayWidth; 
    vector<ofPoint> projectorPoints;
    bool  markProjectorBounds, drawProjectorBounds, applyProjectorHomography;
    void drawProjectorRect();
    cv::Mat projectorHomography;
    ofImage projectorWarp; 
    
    //-------------Box2d
    float xOffset; 
    ofxBox2d                                box2d;
    vector <shared_ptr<ofxBox2dCircle > >   circles;
    vector <shared_ptr<ofxBox2dPolygon> >	polyShapes;
    vector <shared_ptr<ofxBox2dRect   > >	walls;
    vector <shared_ptr<CustomParticle > >   customParticles;
    ofPolyline                              shape;
    void updateBox2DForces(cv::Point2f centroid);
    void createBox2DShape(ofPolyline &daShape);
    vector<ofPoint> scalePolyShape(ofPolyline shapeIn);
    bool gravityOn, wallsOn;
    float circleMin, circleMax, circleFreq;
    void addWalls();
    void saveProjectorPoint(ofVec2f cur, int index);
    void writeProjectorPoints(); 
    

};
