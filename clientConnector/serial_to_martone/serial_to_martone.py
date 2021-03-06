 
import sys, serial, argparse
import numpy as np
from time import sleep
from collections import deque

import matplotlib.pyplot as plt 
import matplotlib.animation as animation
import json
import requests
#usage: python serialplot.py --port COM#
    
     

def main():
  numvalues = 10000


  # create parser
  parser = argparse.ArgumentParser(description="serial plotter")
  # add expected arguments
  parser.add_argument('--port', dest='port', required=True)

  # parse args
  args = parser.parse_args()
  
  strPort = args.port

  print('reading from serial port %s...' % strPort)

  
  ser = serial.Serial(strPort, 115200)
  
  buffer = []
  pres = []
  mag = []
  jsondata = {}
  jsondata["userId"]="23423423423423432"
  jsondata["measurements"] = []
  pressure = np.empty([numvalues])
  magnitude = np.empty([numvalues])
  # print("[")
  for i in range(numvalues):
    print("on number " +str(i))
    serline = ser.readline()
    print(str(serline))
    data = [float(val) for val in serline.split()]
    if(len(data) == 3):
        pres.append(data[0])
        mag.append(data[1])
        jsondata["measurements"].append({"impedance_magnitude":data[1], "impedance_phase":data[2], "pressure":data[0]})

  
  url = "http://capstone-martoneandrew.rhcloud.com/api/capstone/bp/"
  r = requests.post(url, json=jsondata)
  print("status from post: " +str(r.status_code))
  
  x = np.arange(0, numvalues, 1)
  
  
  plt.subplot(2, 1, 1)
  plt.plot(mag)
  plt.title('Pressure and Impedance over Time')
  plt.ylabel('Impedance Magnitude')
  plt.autoscale(enable=True, axis = 'both', tight = None)
  plt.subplot(2, 1, 2)
  plt.plot(pres)
  plt.ylabel('Pressure')
  plt.xlabel('sample')
  plt.autoscale(enable=True, axis = 'both', tight = None)
  plt.show()

  
  

# call main
if __name__ == '__main__':
  main()