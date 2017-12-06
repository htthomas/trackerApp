#UAA LectureTrack Software
#    Cenek's CS Capstone
#    Fall Semester 2017
#
#Coded by:
#    Henry Thomas
#    Tevin Gladden
#    Devon Olsen
#
#REQUIREMENTS:
#    python 2.x.x
#    pip install opencv-python

import cv2
import getpass
import sys
import telnetlib
import threading
import thread

from Tkinter import *

class TrackingApp:
    def __init__(self):
        pass
    def displayStart(self, master):
        print ("Start Flag", master.start_flag)
        
class TrackingWindow:
    def __init__(self, start_flag, exit_flag):
        self.start_flag = start_flag
        self.exit_flag = exit_flag
        #exit_flag = 0
        
        #Window Creation
        self.root = Tk()
        self.root.title("Lecturer-Tracking App")
        self.root.geometry("300x300")
        self.can = Canvas(self.root, bg = 'red', height = 150, width = 150)
        self.can.place(relx = 0.5, rely = 0.5, anchor = CENTER)
        
        #button creation
        #start = Button(self.can, text="Automatic", width=50, command=threading.Thread(target=self.start).start())        
        
        start = Button(self.can, text="Begin Tracking", width=50, command=lambda: self.begin())
        start.pack()
        
        resume = Button(self.can, text="Resume Tracking", width=50, command=lambda: self.start())
        resume.pack()
        
        pause = Button(self.can, text="Pause Tracking", width=50, command=lambda: self.stop())
        pause.pack()
        
        #frame = root
    def getStartFlag(self):
        return self.start_flag
    
    def getStopFlag(self):
        return self.stop_flag
    
    def setStartFlag(self, flag):
        self.start_flag = flag
        
    def setStopFlag(self, flag):
        self.stop_flag = flag 
        
    def start(self):       
        s = threading.Thread(target = self.setStartFlag, args= (1,))
        s.daemon = True
        s.start()
        #self.lectureTrack()
        
    def stop(self):        
        t = threading.Thread(target = self.setStartFlag, args= (0,))   
        t.daemon = True
        t.start()
    def printNumbers(self):
        print self.exit_flag, self.start_flag        
        
    def begin(self):
        track = threading.Thread(target = self.lectureTrack)
        numbers = threading.Thread(target = self.printNumbers)
        numbers.daemon = True
        track.daemon = True 
        track.start()
        numbers.start()
        
    #detect face and return their rectangle
    def detectAndDisplay(self, frame, cascade):
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = cascade.detectMultiScale(gray, 1.3, 7)
        bigface = (0, 0, 0, 0)
        
        for (x, y, w, h) in faces:
            cv2.rectangle(frame, (x, y), (x + w, y + h), (255, 0, 0), 2)
            
        cv2.imshow('imshow', frame)
        
        if len(faces) == 0:
            return None
        
        for i in range(len(faces)):
            face = faces[i];
            if face[2] > bigface[2]:
                bigface = face
                
        return bigface
    
    def trackIt(self, face, frameWidth, frameHeight, frameSize):
        x = face[0]
        y = face[1]
        w = face[2]
        h = face[3]
        centx = x + w / 2
        centy = y + h / 2
        faceSize = w * h
        
        x_weight = abs(200 * (centx / frameWidth - 0.5))
        y_weight = abs(200 * (centy / frameWidth - 0.5))
        lowerLimit = 0.35
        upperLimit = 0.65
    
        if (centx > frameWidth * lowerLimit and centx < frameWidth * upperLimit) or centx == 0:
            x_weight = 0
    
        if (centy > frameHeight * lowerLimit and centy < frameHeight * upperLimit) or centy == 0:
            y_weight = 0
    
        if x_weight >= y_weight and x_weight > 0:
            if centx - frameWidth / 2 > 0:
                return "camera pan right 11\n"
            else:
                return "camera pan left 11\n"
            
        elif x_weight < y_weight and y_weight > 0:
            if centy - frameHeight / 2 > 0:
                return "camera tilt down 7\n"
            else:
                return "camera tilt up 7\n"
            
        elif x_weight == 0 and y_weight == 0:
            if abs(centx - frameWidth / 2) > abs(centy - frameHeight / 2):
                return "camera tilt stop\n"
            else:
                return "camera pan stop\n"
            
    def lectureTrack(self):
        host = "137.229.182.217"
        user = "admin"
        password = "password"#getpass.getpass()
        
        tn = telnetlib.Telnet(host)
        
        tn.read_until("login: ")
        tn.write(user + "\n")
        
        if password:
            tn.read_until("Password: ")
            tn.write(password + "\n")
    
        cascade = cv2.CascadeClassifier("headshoulders.xml")
        capture = cv2.VideoCapture(0)
        #"rtsp://192.168.1.75:554/vaddio-qc-usb-stream"
        
        frameWidth = capture.get(3)
        frameHeight = capture.get(4) 
        frameSize = frameWidth * frameHeight
        
        face = None
        prvface = None
        cmd = ""
        prvcmd = "camera pan stop"
    
        j = 0 #frame counter
        nodetects = 0 #counter for frames with no detections

        #while inside while to maintain loop and active program
        #while self.getStartFlag() == 1 or while self.getStartFlag() == 1
        while self.exit_flag == 0:
            print self.exit_flag, self.start_flag
            
                
            ret, frame = capture.read()
            j += 1
            
            if frame is not None:
                face = self.detectAndDisplay(frame, cascade)
                    
                if face is None: #if no face detected
                    face = prvface
                    nodetects += 1
                    
                    if nodetects == 150: #seconds to wait = nodetects / framerate
                        print ("tracking failure, recovering...")
                        tn.write("camera home\n")
                        
                else:
                    nodetects = 0
                    
                    if j % 1 == 0: #set command freq here
                        cmd = self.trackIt(face, frameWidth, frameHeight, frameSize);
            
                        if (cmd != prvcmd):
                            if self.start_flag == 1:
                                tn.write(cmd)
                        print(cmd)
                        prvcmd = cmd
                        prvface = face    
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    
        print (tn.read_all())
        cap.release()
        cv2.destroyAllWindows()                        
        
        
#if __name__ == '__main__':
    #if len(sys.argv) == 5:
        #ip = argv[1]
        #port = argv[2]
        #acct = argv[3]
        #pw = argv[4]
    #else:
        #ip = raw_input("IP: ") #192.168.1.75
        #ip = raw_input("PORT: ") #23
        #ip = raw_input("ACCT: ") #admin
        #ip = raw_input("PW: ") #password
#app = TrackingApp()
window = TrackingWindow(1, 0)
window.root.mainloop()
    
