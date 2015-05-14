#pragma once

#include "ofMain.h"
#include "ofMath.h"
#include "ofSerial.h"

// 3rd party addons
#include "ofxOsc.h"
#include "ofxCsv.h"
#include "ofxMidi.h"

//#define HOST "localhost"
#define HOST "192.168.1.255"
#define PORT 57120

#define H_MARGIN 100
#define W_MARGIN 100
#define TH_MARGIN 20
#define TW_MARGIN 20

#define SKIP 20
#define FRAME_RATE 20

using namespace wng;

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
	   
        vector<vector<double> > data;
        int num_series; // "columns"
        int num_elements; // "rows"

        vector<ofColor> d_color;
        vector<ofColor> u_d_color;

        int counter;
        int counter_skip;
        ofxCsv csv_reader;

        // OSC mechanism
		ofxOscSender sender;        
};
