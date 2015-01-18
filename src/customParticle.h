//
//  customParticle.h
//  PS3_Homography
//
//  Created by Seth Hunter on 1/17/15.
//
//

#ifndef PS3_Homography_customParticle_h
#define PS3_Homography_customParticle_h
#include "ofxBox2d.h"

// ------------------------------------------------- a simple extended box2d circle
class CustomParticle : public ofxBox2dCircle {
    
public:
    CustomParticle() {
    }
    ofColor color;
    void draw() {
        float radius = getRadius();
        
        glPushMatrix();
        glTranslatef(getPosition().x, getPosition().y, 0);
        
        ofSetColor(color.r, color.g, color.b);
        ofFill();
        ofCircle(0, 0, radius);	
        
        glPopMatrix();
        
    }
};



#endif
