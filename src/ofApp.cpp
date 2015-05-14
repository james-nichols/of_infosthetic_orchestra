#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    counter = 0;
    
    //for (int i=0;i<NUM_DATA;i++)
    //    data.push_back(ofRandom(0.0, 1.0));
    csv_reader.loadFile(ofToDataPath("/Users/james/projects/infosthetic_orchestra/mtgoxAUD.csv"));
    num_elements = csv_reader.numRows;
    num_series = csv_reader.data[0].size(); // Risky - assuming constant num of rows through file
    
    for (int j=0;j<num_series;j++) {
        
        vector<double> col;

        // We give each data series a random colour
        int hue = float(j*255)/float(num_series); // ofRandom(0,255);
        d_color.push_back(ofColor::fromHsb(hue, 240, 240));
        u_d_color.push_back(ofColor::fromHsb(hue, 80, 80));
        
        for (int i=0;i<num_elements;i++) {
            col.push_back(atof(csv_reader.data[i][j].c_str()));
        }
        data.push_back(col);
    }
    

    // Now normalise the data
    for (int i=1;i<num_series;i++) {
        double max = *(max_element(data[i].begin(), data[i].end()));
        for (int j=0;j<num_elements;j++) {
            data[i][j] = data[i][j] / max;
        }
    }

    ofSetWindowTitle("Infosthetic Orchestra Data Broadcaster and Visualiser");
    ofLog() << "Estimated time to finish cycle: " << float(num_elements) / float(SKIP * FRAME_RATE);
    ofDisableAntiAliasing();
    ofSetFrameRate(FRAME_RATE);

	sender.setup(HOST, PORT);    
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackgroundGradient(ofColor(70), ofColor(0));

    ofSetColor(255, 255, 255);

    for (int j=1;j<num_series;j++) {
        ofMesh graph;
        graph.setMode(OF_PRIMITIVE_LINE_STRIP);

        // Write legend
        ofSetColor(d_color[j]);
    	ofDrawBitmapString("Data "+ofToString(j)+": "+ofToString(data[j][counter]), TW_MARGIN, TH_MARGIN + j*12);

        for (int i=0;i<num_elements;i+=SKIP) {
            if (i<counter)
                graph.addColor(d_color[j]);
            else 
                graph.addColor(u_d_color[j]);
            float x = float((ofGetWidth()-2*W_MARGIN) * i) / float(num_elements) + float(W_MARGIN);
            float y = ofGetHeight() - ((ofGetHeight()-2*H_MARGIN) * data[j][i] + float(H_MARGIN));
            graph.addVertex(ofVec2f(x,y)); 
        }
        graph.draw();

        // Send OSC messages...
		ofxOscMessage m;
		m.setAddress("/data" + ofToString(j));
		m.addFloatArg(data[j][counter]);
		sender.sendMessage(m);

        // Send MIDI
        
        // Send serial/ardiuno CV
    }

    // Draw little circles that follow the data
    for (int j=1;j<num_series;j++) {
        float x = float((ofGetWidth()-2*W_MARGIN) * counter) / float(num_elements) + float(W_MARGIN);
        float y = ofGetHeight() - ((ofGetHeight()-2*H_MARGIN) * data[j][counter] + float(H_MARGIN));

        ofSetColor(d_color[j]);
        ofNoFill();
        ofCircle(x, y, 3);
        ofSetColor(ofColor(255,255,255,80));
        ofFill();
        ofCircle(x, y, 2);
 
    }

    ofSetColor(ofColor(255));
    ofDrawBitmapString("Data per second: " + ofToString(ofGetFrameRate()), TW_MARGIN, TH_MARGIN + (num_series+1)*12);
    
    counter = (counter + SKIP) % num_elements;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
