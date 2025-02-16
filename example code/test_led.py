
import serial
import json
import fastapi
import time

ser = None
app = fastapi.FastAPI()

dictPattern = {"fullColor", "rainbow", "chase", "colorWipe", "fadeToColor", "runningLights", "fadeToBlack"}

@app.post("/setLEDs")
async def full_color(stripOn: bool = True, patternName: str = dictPattern, color_Red: int = 0, color_Green: int = 0, color_Blue: int = 0, brightness: int = 255, speed: int = 200, colorStartR: int = 0, colorStartG: int = 0, colorStartB: int = 0, colorEndR: int = 0, colorEndG: int = 0, colorEndB: int = 0):
    return send_command(stripOn= stripOn, pattern= patternName, colorR = color_Red, colorG = color_Green, colorB = color_Blue, brightness = brightness, speed= speed, colorStartR= colorStartR, colorStartG=colorStartG, colorStartB=colorStartB, colorEndR=colorEndR, colorEndG=colorEndG, colorEndB=colorEndB)

@app.post("/help")
async def help():
    try:
        print("connecting...")

        ser = serial.Serial('COM3', 115200, timeout=5)

        print("connected")
        
    except serial.SerialException as e:
        return {"status": "error1", "message": str(e)}

    try:
        help_command = "help"
        print("Sending help command...")
    
        ser.write(help_command.encode())
        time.sleep(0.5)

        response = ""
        while True:
            response += ser.readline().decode('utf-8').strip()
            if not response.__contains__("~"):
                continue
            break

        print("response: " + response)
        print("Reading response...")
        


        #response = ser.readline().decode().strip()
        
        print("Response received:", response)
        
        return response
    finally:
        ser.close()    

@app.post("/Status_barrier")
async def getBerrierStatus():
    try:
        print("connecting...")
        ser = serial.Serial('COM3', 115200, timeout=5)
        print("connected")   
    except serial.SerialException as e:
        return {"status": "error1", "message": str(e)}
    try:
        command = "Status barrier"
        print("Sending command...")
        
        ser.write(command.encode())
        time.sleep(0.5)
        
        response = ser.readline().decode('utf-8').strip()
        print("response: " + response)
        print("Reading response...")
        
        return response
    finally:
        ser.close()    


        
@app.post("/Barrier_Threashold")
async def getBerrierStatus(lightBarrierThreshold: int = 200):
    try:
        print("connecting...")
        ser = serial.Serial('COM3', 115200, timeout=5)
        print("connected")   
    except serial.SerialException as e:
        return {"status": "error1", "message": str(e)}
    try:
        command = "{\"lightBarrierThreshold\": " + lightBarrierThreshold + "}"
        print("Sending command...")
        
        ser.write(command.encode())
        time.sleep(0.5)
        
        response = ser.readline().decode('utf-8').strip()
        print("response: " + response)
        print("Reading response...")
        
        return response
    finally:
        ser.close()    

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
        
        time.sleep(1) # Adjust as needed
        response =  ser.readline().decode('utf-8').strip()
        
        ser.close()
        return {"status": "success", "response": response}
    except serial.SerialException as e:
        ser.close()  # Close even if there's an error
        return {"status": "error2", "message": str(e)}