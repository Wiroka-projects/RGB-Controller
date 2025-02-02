
"""
start serial console to com3 at 115200 baud rate
then test following json codes to send to the device

  Serial.println("{\"stripOn\":true, \"color\":[155,0,0], \"brightness\":90, \"pattern\":{\"patternName\":\"fullColor\"}}");
  Serial.println("{\"stripOn\":true, \"color\":[0,0,150], \"brightness\":150, \"pattern\":{\"patternName\":\"fullColor\"}}");
  Serial.println("{\"stripOn\":true, \"brightness\":90, \"pattern\":{\"patternName\":\"rainbow\", \"speed\":200}}");
  Serial.println("{\"stripOn\":true, \"brightness\":150, \"pattern\":{\"patternName\":\"rainbow\", \"speed\":1000}}");
  Serial.println("{\"stripOn\":true, \"color\":[150,0,0], \"brightness\":150, \"pattern\":{\"patternName\":\"chase\", \"speed\":50}}");
  Serial.println("{\"stripOn\":true, \"color\":[0,0,150], \"brightness\":150, \"pattern\":{\"patternName\":\"chase\", \"speed\":200}}");
  Serial.println("{\"stripOn\":true, \"color\":[255,0,0], \"brightness\":100, \"pattern\":{\"patternName\":\"colorWipe\", \"speed\":210}}");
  Serial.println("{\"stripOn\":false, \"color\":[255,255,0], \"brightness\":100, \"pattern\":{\"patternName\":\"colorWipe\", \"speed\":1000}}");
  Serial.println("{\"stripOn\":true, \"pattern\":{\"patternName\":\"colorWipe\"}}");
  Serial.println("{\"stripOn\":true, \"color\":[55,10,100], \"brightness\":40, \"pattern\":{\"patternName\":\"fadeToColor\", \"colorStart\":[100,10,0], \"colorEnd\":[0,10,100], \"speed\":1000}}");
  Serial.println("{\"stripOn\":true, \"color\":[55,10,100], \"brightness\":80, \"pattern\":{\"patternName\":\"fadeToColor\", \"colorStart\":[50,50,0], \"colorEnd\":[0,100,150], \"speed\":100}}");
  Serial.println("{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"runningLights\", \"speed\":500}}");
  Serial.println("{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"runningLights\", \"speed\":200}}");
  Serial.println("{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"fadeToBlack\", \"speed\":140}}");
  Serial.println("{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"fadeToBlack\", \"speed\":20}}");
  Serial.println("{\"stripOn\":false}");
"""

import serial
import json
import fastapi
import time

ser = None
app = fastapi.FastAPI()

dictPattern = {"fullColor", "rainbow", "chase", "colorWipe", "fadeToColor", "runningLights", "fadeToBlack"}

@app.post("/setLEDs")
async def full_color(stripOn: bool = True, patternName: str = None, color_Red: int = 0, color_Green: int = 0, color_Blue: int = 0, brightness: int = 255, speed: int = 200, colorStartR: int = 0, colorStartG: int = 0, colorStartB: int = 0, colorEndR: int = 0, colorEndG: int = 0, colorEndB: int = 0):
    return send_command(stripOn= stripOn, pattern= patternName, colorR = color_Red, colorG = color_Green, colorB = color_Blue, brightness = brightness, speed= speed, colorStartR= colorStartR, colorStartG=colorStartG, colorStartB=colorStartB, colorEndR=colorEndR, colorEndG=colorEndG, colorEndB=colorEndB)


def send_command(stripOn: bool = None, pattern: str = None, colorR: int = None, colorG: int = None, colorB: int= None, brightness: int = None, speed: int = None, colorStartR: int = None, colorStartG: int = None, colorStartB: int = None, colorEndR: int = None, colorEndG: int = None, colorEndB: int = None):
    global ser
    
    print("Serial port is not open. Attempting to open...")
    try:
        ser = serial.Serial('COM3', 115200)
    except serial.SerialException as e:
        return {"status": "error1", "message": str(e)}

    data = {}
    patternData = {}
    if stripOn is not None:
        data['stripOn'] = stripOn
    if colorR is not None and colorG is not None and colorB is not None:
        data['color'] = [colorR, colorG, colorB]
    if brightness is not None:
        data['brightness'] = brightness
    if pattern is not None:
        patternData['patternName'] = pattern
    if speed is not None:
        patternData['speed'] = speed
    if colorStartR is not None and colorStartG is not None and colorStartB is not None:
        patternData['colorStart'] = [colorStartR, colorStartG, colorStartB]
    if colorEndR is not None and colorEndG is not None and colorEndB is not None:
        patternData['colorEnd'] = [colorEndR, colorEndG, colorEndB]
    
    if patternData:
        data['pattern'] = patternData


    try:
        command = json.dumps(data) + '\n'

        ser.write(command.encode('utf-8'))
        print(command)
        time.sleep(0.5) # Adjust as needed
        
        response = ser.readline().decode('utf-8').strip()
        
        ser.close()
        return {"status": "success", "response": response}
    except serial.SerialException as e:
        ser.close()  # Close even if there's an error
        return {"status": "error2", "message": str(e)}