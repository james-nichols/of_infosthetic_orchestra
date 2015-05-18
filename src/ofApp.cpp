#include "ofApp.h"
#include <math.h>
#include <time.h>

//--------------------------------------------------------------
void ofApp::setup(){
    counter = 0;
    ofSetDataPathRoot("./");  
    // Load the data from a CSV file...
    csv_reader.loadFile(ofToDataPath("../Resources/mtgoxAUD.csv"));
    num_series = csv_reader.data[0].size(); // Risky - assuming constant num of cols through file
    num_elements = 0; 
    
    for (int j=0;j<num_series;j++) {
        
        vector<double> col;

        // We give each data series a colour
        int hue = float(j*255)/float(num_series); // ofRandom(0,255);
        d_color.push_back(ofColor::fromHsb(hue, 240, 240));
        u_d_color.push_back(ofColor::fromHsb(hue, 80, 80));
        
        for (int i=0;i<csv_reader.numRows;i+=SKIP) {
            if (j==1) col.push_back(log(atof(csv_reader.data[i][j].c_str())));
            else col.push_back(atof(csv_reader.data[i][j].c_str()));
            if (j==0) num_elements++;
        }
        data.push_back(col);
    }
    

    // Now normalise the data
    for (int i=1;i<num_series;i++) {
        double max = *(max_element(data[i].begin(), data[i].end()));
        double min = *(min_element(data[i].begin(), data[i].end()));
        for (int j=0;j<num_elements;j++) {
            data[i][j] = ofNormalize(data[i][j], min, max);
        }
    }

    ofLog() << "Estimated time to finish cycle: " << float(num_elements) / float(FRAME_RATE);

    // Set up the OSC osc_sender
    osc_sender.setup(HOST, PORT);    
   
    // Set up the MIDI osc_sender
    midi_channel = 1;
    midi_note = 30;
    midi_velocity = 127;
    
    midi_out.listPorts();
    ofLog() << "Opening MIDI port " << 0 << ", sending to channel " << midi_channel;
    midi_out.openPort(0);

    ofSetWindowTitle("Infosthetic Orchestra Data Broadcaster and Visualiser");
    ofDisableAntiAliasing();
    ofSetFrameRate(FRAME_RATE);

    // Set up the arduino for CV output (via PWM)
	ard.connect("/dev/tty.usbserial-A9007W2m", 57600);
    ofAddListener(ard.EInitialized, this, &ofApp::setupArduino);
    bSetupArduino = false;	// flag so we setup arduino when its ready, you don't need to touch this :)
     
	//serial.setup("/dev/tty.usbserial-A9007W2m", 57600); // mac osx example
}

//--------------------------------------------------------------
void ofApp::setupArduino(const int & version) {
	
	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &ofApp::setupArduino);
    
    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    
    // print firmware name and version to the console
    ofLogNotice() << ard.getFirmwareName(); 
    ofLogNotice() << "firmata v" << ard.getMajorFirmwareVersion() << "." << ard.getMinorFirmwareVersion();
    
    // set pin D11 as PWM (analog output)
	ard.sendDigitalPinMode(9, ARD_PWM);
	ard.sendDigitalPinMode(10, ARD_PWM);
	ard.sendDigitalPinMode(11, ARD_PWM);
	
}


//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackgroundGradient(ofColor(50), ofColor(0));

    ofSetColor(255, 255, 255);

    // Update the arduino
    ard.update();
    
    for (int j=num_series-1;j>0;j--) {
        ofMesh graph;
        graph.setMode(OF_PRIMITIVE_LINE_STRIP);

        // Write legend
        ofSetColor(d_color[j]);
    	ofDrawBitmapString("Data "+ofToString(j)+": "+ofToString(data[j][counter]), TW_MARGIN, TH_MARGIN + j*12);

        for (int i=0;i<num_elements;i++) {
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
		osc_sender.sendMessage(m);

        // Send MIDI
        midi_out.sendNoteOff(midi_channel, midi_note, midi_velocity); // Cancel previous note...
        midi_note = ofMap(data[j][counter], 0.0, 1.0, MIDI_MIN, MIDI_MAX);
        midi_out.sendNoteOn(midi_channel, midi_note, midi_velocity); // Send new note
        // In case we want to send pitch-bend data...
        //midi_out.sendPitchBend(channel, bend);
        
        // Send serial/ardiuno CV
        if (bSetupArduino) {
            int pin = 8+j; // Super hacky: series 1 gives us pin 9, 2 gives us pin 10, 3 gives 11 (the only available pins...)
		    ard.sendPwm(pin, (int)(256 * data[j][counter]));   // pwm...
        }

        //serial.writeByte(8+j);
		//serial.writeByte((int)(256 * data[j][counter]));   // pwm...
    }

    // Draw little circles that follow the data
    for (int j=num_series-1;j>0;j--) {
        float x = float((ofGetWidth()-2*W_MARGIN) * counter) / float(num_elements) + float(W_MARGIN);
        float y = ofGetHeight() - ((ofGetHeight()-2*H_MARGIN) * data[j][counter] + float(H_MARGIN));

        ofSetColor(d_color[j]);
        ofNoFill();
        ofCircle(x, y, 3);
        ofSetColor(ofColor(255,255,255,80));
        ofFill();
        ofCircle(x, y, 2);
 
    }

    // Send the date data
    // Send OSC messages...
    time_t time = int(data[0][counter]);
    struct tm *tm = localtime(&time);
    ofxOscMessage y;
    y.setAddress("/year");
    y.addIntArg(tm->tm_year+1900);
    osc_sender.sendMessage(y);

    ofxOscMessage m;
    m.setAddress("/month");
    m.addIntArg(tm->tm_mon+1);
    osc_sender.sendMessage(m);
    
    ofxOscMessage d;
    d.setAddress("/day");
    d.addIntArg(tm->tm_mday);
    osc_sender.sendMessage(d);

    ofSetColor(ofColor(255));
    ofDrawBitmapString("Data per second: " + ofToString(ofGetFrameRate()), TW_MARGIN, TH_MARGIN + (num_series+1)*12);
    ofDrawBitmapString("Date: " + ofToString(tm->tm_mday) + "-" + ofToString(tm->tm_mon+1) + "-" + ofToString(tm->tm_year+1900), TW_MARGIN, TH_MARGIN + (num_series+2)*12);
    
    counter = (counter + 1) % num_elements;
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
