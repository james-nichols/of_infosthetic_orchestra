#include "ofApp.h"
#include <math.h>
#include <time.h>

//--------------------------------------------------------------
void ofApp::setup(){
    counter = 0;
    bRun = false;
    bDraw = true;
    ofSetDataPathRoot("./");  
    // Load the data from a CSV file...
    csv_reader.loadFile(ofToDataPath("../Resources/mtgoxAUD.csv"));
    num_series = csv_reader.data[0].size(); // Risky - assuming constant num of cols through file
    num_elements = 0; 
     
    for (int j=0;j<num_series;j++) {
        // We give each data series a colour
        int hue = fmod(float((j-1)*255)/float(num_series),float(255.0)); // ofRandom(0,255);
        d_color.push_back(ofColor::fromHsb(hue, 240, 240));
        u_d_color.push_back(ofColor::fromHsb(hue, 80, 80));
        
        vector<double> col;
        vector<double> col_orig;
        for (int i=0;i<csv_reader.numRows;i+=SKIP) {
            if (j==1) col.push_back(log(atof(csv_reader.data[i][j].c_str())));
            else col.push_back(atof(csv_reader.data[i][j].c_str()));
            if (j==0) num_elements++;
            
            col_orig.push_back(atof(csv_reader.data[i][j].c_str()));
        }
        data.push_back(col);
        data_original.push_back(col_orig);
    }
    
    for (int j=0;j<num_elements;j++) {
        data[2][j] = data[2][j] * data[1][j] * data[1][j];
    } 

    // Now normalise the data
    for (int i=1;i<num_series;i++) {
        double max = *(max_element(data[i].begin(), data[i].end()));
        double min = *(min_element(data[i].begin(), data[i].end()));
        for (int j=0;j<num_elements;j++) {
            data[i][j] = ofNormalize(data[i][j], min, max);
        }
        // Finish with a (low) bang...
        data[i].push_back(0.0);
    }
    data[0].push_back(*(max_element(data[0].begin(), data[0].end())));


    ofLog() << "Estimated time to finish cycle: " << float(num_elements) / float(FRAME_RATE);

    // Set up the OSC osc_sender
    osc_sender.setup(HOST, PORT);    
   
    // Set up the MIDI osc_sender
    midi_channel = 1;
    midi_note = 30;
    midi_arp_note = 30;
    midi_velocity = 127;
    
    midi_out.listPorts();
    ofLog() << "Opening MIDI port " << 0 << ", sending to channel " << midi_channel;
    midi_out.openPort(0);
    midi_out_2.openPort(1);

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

    time_t time = int(data[0][counter]);
    struct tm *tm = localtime(&time);
    
    if (bDraw) { 
    for (int j=num_series-1;j>0;j--) {
        ofMesh graph;
        ofMesh graph_double;
        graph.setMode(OF_PRIMITIVE_LINE_STRIP);
        graph_double.setMode(OF_PRIMITIVE_LINE_STRIP);

        // Write legend
        ofSetColor(d_color[j]);
        if (j==1) ofDrawBitmapString("Data 1, AUD/BTC (MtGox): $"+ofToString(data_original[j][counter]), TW_MARGIN, TH_MARGIN + j*12);
        if (j==2) ofDrawBitmapString("Data 2, Price weighted volume: "+ofToString(data_original[j][counter]) + " BTC", TW_MARGIN, TH_MARGIN + j*12);
        //ofDrawBitmapString("Data "+ofToString(j)+": "+ofToString(data_original[j][counter]), TW_MARGIN, TH_MARGIN + j*12);

        for (int i=0;i<num_elements;i++) {
            if (i<counter) {
                graph.addColor(d_color[j]);
                graph_double.addColor(d_color[j]);
            }
            else {
                graph.addColor(u_d_color[j]);
                graph_double.addColor(u_d_color[j]);
            }
            float x = float((ofGetWidth()-2*W_MARGIN) * i) / float(num_elements) + float(W_MARGIN);
            float y = ofGetHeight() - ((ofGetHeight()-2*H_MARGIN) * data[j][i] + float(H_MARGIN));
            graph.addVertex(ofVec2f(x,y)); 
            graph_double.addVertex(ofVec2f(x,y+1)); 
        }
        graph.draw();
        graph_double.draw();
    }

    // Draw little circles that follow the data
    for (int j=num_series-1;j>0;j--) {
        float x = float((ofGetWidth()-2*W_MARGIN) * counter) / float(num_elements) + float(W_MARGIN);
        float y = ofGetHeight() - ((ofGetHeight()-2*H_MARGIN) * data[j][counter] + float(H_MARGIN));

        ofSetColor(d_color[j]);
        ofNoFill();
        ofCircle(x, y, 5);
        ofSetColor(ofColor(255,255,255,80));
        ofFill();
        ofCircle(x, y, 4);
 
    }

    ofSetColor(ofColor(255));
    ofDrawBitmapString("Data output rate: " + ofToString(round(ofGetFrameRate())) + " Hz", TW_MARGIN, TH_MARGIN + (num_series+1)*12);
    ofDrawBitmapString("Date: " + ofToString(tm->tm_mday) + "-" + ofToString(tm->tm_mon+1) + "-" + ofToString(tm->tm_year+1900), TW_MARGIN, TH_MARGIN + (num_series+2)*12);
    }
    if (bRun && counter < num_elements) {

        // Send OSC and MIDI messages
        
        // Update the arduino
        ard.update();
        
        for (int j=num_series-1;j>0;j--) {
            // Send OSC messages...
            ofxOscMessage m;
            m.setAddress("/data" + ofToString(j));
            m.addFloatArg(data[j][counter]);
            osc_sender.sendMessage(m);            
            // Send serial/ardiuno CV
            if (bSetupArduino) {
                int pin = 8+j; // Super hacky: series 1 gives us pin 9, 2 gives us pin 10, 3 gives 11 (the only available pins...)
                ard.sendPwm(pin, (int)(256 * data[j][counter]));   // pwm...
            }
        }

        // Send MIDI
        midi_out.sendNoteOff(midi_channel, midi_arp_note, midi_velocity); // Cancel previous note...
        //midi_out_2.sendNoteOff(midi_channel, midi_note, midi_velocity); // Cancel previous note...
        midi_note = ofMap(data[1][counter], 0.0, 1.0, MIDI_MIN, MIDI_MAX);
        midi_arp_note = int(ofMap(data[1][counter], 0.0, 1.0, MIDI_MIN, MIDI_MAX)) + int(ofMap(data[2][counter], 0.0, 1.0, 0, 12));
        midi_velocity = ofMap(data[2][counter], 0.0, 1.0, 64, 127);
        midi_out.sendNoteOn(midi_channel, midi_note, midi_velocity); // Send new note
        midi_out_2.sendNoteOn(midi_channel, midi_arp_note, midi_velocity); // Send new note
        // In case we want to send pitch-bend data...
        //midi_out.sendPitchBend(channel, bend);

        // Send the date data
        // Send OSC messages...
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
        
        counter++;
    }
    if (!bRun && counter < num_elements && bDraw) 
        if (counter == 0)
            ofDrawBitmapString("Press 'r' to run", TW_MARGIN, TH_MARGIN + (num_series+5)*12);
        else
            ofDrawBitmapString("Paused. Press 'r' to run", TW_MARGIN, TH_MARGIN + (num_series+5)*12);
    //if (!bDraw)
    //    ofDrawBitmapString("d", TW_MARGIN, TH_MARGIN + (num_series+6)*12);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key=='r')
        bRun = !bRun;
    if (key=='d')
        bDraw = !bDraw; 
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    if (x > W_MARGIN && x < (ofGetWidth() - W_MARGIN))
        counter = num_elements * (x-W_MARGIN) / (ofGetWidth() - 2 * W_MARGIN);
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
