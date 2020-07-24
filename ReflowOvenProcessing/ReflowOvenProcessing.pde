// Graphing sketch
 
 
 // This program takes ASCII-encoded strings
 // from the serial port at 9600 baud and graphs them. It expects values in the
 // range 0 to 1023, followed by a newline, or newline and carriage return
 
 // Created 20 Apr 2005
 // Updated 18 Jan 2008
 // by Tom Igoe
 // This example code is in the public domain.
 
 // gridlines
 
// grid specific vars
// grid spacing in pixels
//int GRID_MINOR_SPACING = 10;
int GRID_MAJOR_SPACING = 100;
// gridline colors
//color GRID_MINOR_COLOR = color(147, 161, 247, 127);
color GRID_MAJOR_COLOR = color(0, 19, 137, 127);
// gridline weights in pixels
//int GRID_MINOR_WEIGHT = 1;
int GRID_MAJOR_WEIGHT = 2;
// when true, grid is drawn, else grid is not
boolean GRID_DRAW = true;
 
 
 
 import processing.serial.*;
 
 Serial myPort;        // The serial port
 int xPos = 1;         // horizontal position of the graph
 int rectX, rectY;      // Position of square button
int circleX, circleY;  // Position of circle button
int rectSize = 20;     // Diameter of rect
int circleSize = 20;   // Diameter of circle
color rectColor, circleColor, baseColor;
color rectHighlight, circleHighlight;
color currentColor;
boolean rectOver = false;
boolean circleOver = false;
int recttime;

// grid code
public void drawGrid() {
  if (GRID_DRAW) {
   // int num_minor_x = width/GRID_MINOR_SPACING;
   // int num_minor_y = height/GRID_MINOR_SPACING;
    int num_major_x = width/GRID_MAJOR_SPACING;
    int num_major_y = height/GRID_MAJOR_SPACING;
   
    pushStyle();
    strokeCap(PROJECT);
    
    /*
    strokeWeight(GRID_MINOR_WEIGHT);
    stroke(GRID_MINOR_COLOR);
    for (int i = 0; i < num_minor_y; i++) {
      int y = i * GRID_MINOR_SPACING;
      line(0, y, width, y);
    }
    for (int i = 0; i < num_minor_x; i++) {
      int x = i * GRID_MINOR_SPACING;
      line(x, 200, x, 600); 
    }*/
    
    strokeWeight(GRID_MAJOR_WEIGHT);
    stroke(GRID_MAJOR_COLOR);
    for (int i = 0; i < num_major_y; i++) {
      int y = i * GRID_MAJOR_SPACING;
      line(0, y, width, y);
    }
    for (int i = 0; i < num_major_x; i++) {
      int x = i * GRID_MAJOR_SPACING;
      line(x, 200, x, 600);
    }
    popStyle();
    
      textSize(14);
      fill(255);
      stroke(255);
       text("100", 5, 480, 70, 300);  // Text wraps within text box
       
          textSize(14);
      fill(255);
      stroke(255);
       text("200", 5, 380, 70, 300);  // Text wraps within text box
       
             textSize(14);
      fill(255);
      stroke(255);
       text("300", 5, 280, 70, 300);  // Text wraps within text box
       
             textSize(14);
      fill(255);
      stroke(255);
       text("400", 5, 180, 70, 300);  // Text wraps within text box
       
            
  }
  
}

 void setup () {
 // set the window size:
 size(800, 600);     
  frameRate(30);
 recttime = millis();

  rectColor = color(180, 220, 192);
  rectHighlight = color(250, 240, 17);
  circleColor = color(214, 80, 65);
  circleHighlight = color(165, 29, 13);
  baseColor = color(31, 155, 85);
  currentColor = baseColor;
  circleX = 180;
  circleY = 100;
  rectX = 100;
  rectY = 90; 
 
 // List all the available serial ports
 println(Serial.list());
 // I know that the first port in the serial list on my mac
 // is always my  Arduino, so I open Serial.list()[0].
 // Open whatever port is the one you're using.
 myPort = new Serial(this, Serial.list()[1], 115200);
 // don't generate a serialEvent() unless you get a newline character:
 myPort.bufferUntil('\n');
 // set inital background:
 background(0);
 }
 
 
 void draw () {
   
  
  
  // draw grid on top -- won't draw when GRID_DRAW == false
  drawGrid();

     update(mouseX, mouseY);
 if (rectOver) {
    fill(rectHighlight);
  } else {
    fill(rectColor);
  }
  stroke(255);
  rect(rectX, rectY, rectSize, rectSize);
  
  if (circleOver) {
    fill(circleHighlight);
  } else {
    fill(circleColor);
  }
  stroke(255);
  ellipse(circleX, circleY, circleSize, circleSize);
  
  textSize(14);
  fill(255);
  stroke(255);
   text("Reflow", 90, 70, 70, 300);  // Text wraps within text box

  textSize(14);
   fill(165, 29, 13);
 stroke(165, 29, 13);
   text("Stop", 170, 70, 70, 300);  // Text wraps within text box


   /* if(millis() > recttime + 5000) {
      recttime = millis(); // reset start time
      stroke(0);
       fill(0);
       rect(100, 200, 80, 100);
   }
                                      */

 }
 
 void update(int x, int y) {
  if ( overCircle(circleX, circleY, circleSize) ) {
    circleOver = true;
    rectOver = false;
  } else if ( overRect(rectX, rectY, rectSize, rectSize) ) {
    rectOver = true;
    circleOver = false;
  } else {
    circleOver = rectOver = false;
  }
  
 }
 
 
 
 void serialEvent (Serial myPort) {
 // get the ASCII string:
 String inString = myPort.readStringUntil('\n');
 
 if (inString != null) {
 // trim off any whitespace:
 inString = trim(inString);
 // convert to an int and map to the screen height:
 float inByte = float(inString);
 inByte = map(inByte, 0, 600, 0, height);
 
 // draw the line:
 stroke(116, 154, 255);
 line(xPos, height, xPos, height - inByte);
  
    
 // at the edge of the screen, go back to the beginning:
 if (xPos >= width) {
 xPos = 0;
 background(0);
 }
 else {
 // increment the horizontal position:
 xPos++;
    

 }
 }
 
 }
 

  
  
 void mousePressed() {
  if (circleOver) {
    myPort.write('o');   
    circleColor = color(144, 13, 33);
    rectColor = color(180, 220, 192);
  }
  if (rectOver) {
    myPort.write('g');  
    circleColor = color(214, 80, 65);
    rectColor = color(250, 250 , 20);

 
  }
}

boolean overRect(int x, int y, int width, int height)  {
  if (mouseX >= x && mouseX <= x+width && 
      mouseY >= y && mouseY <= y+height) {
    return true;
  } else {
    return false;
  }
}

boolean overCircle(int x, int y, int diameter) {
  float disX = x - mouseX;
  float disY = y - mouseY;
  if (sqrt(sq(disX) + sq(disY)) < diameter/2 ) {
    return true;
  } else {
    return false;
  }
}
