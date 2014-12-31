// =============================================================================
//
// Copyright (c) 2014 Christopher Baker <http://christopherbaker.net>
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


class ofApp: public ofBaseApp
{
public:
    
    //-------standard functions
    void setup();
    void update();
    void draw();
    void exit();
    void mousePressed(int x, int y, int button);
    void mouseDragged(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void keyPressed(int key);
    
    //---------homography functions
    bool movePoint(vector<ofVec2f>& points, ofVec2f point, int LeftOrRight);
    void drawPoints(vector<ofVec2f>& points);
    void updatePostions();
    void clearPoints();
    void saveXMLPoints(ofVec2f cur);
    void pushXMLPoint(ofVec2f point, int index, int LorR);
    
    //---------added for ofxUI functionality
    void mouseMoved(int x, int y );
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    ofxUISuperCanvas *gui0; 
    ofxUISuperCanvas *gui1;
    ofxUISuperCanvas *gui2;
    void guiEvent(ofxUIEventArgs &e);
    int fiveFrames; 
    
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
    cv::Mat homography;
    
    //------------Tracking
    void drawTracker(); 
    ofxCv::ContourFinder contourFinder;
    float threshold;
    bool showTracker;

    

};
