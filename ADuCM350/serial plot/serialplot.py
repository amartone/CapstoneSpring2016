 
import sys, serial, argparse
import numpy as np
from time import sleep
from collections import deque

import matplotlib.pyplot as plt 
import matplotlib.animation as animation

#usage: python serialplot.py --port COM#
    
    
# plot class
class AnalogPlot:
  # constr
  def __init__(self, strPort, maxLen):
      # open serial port
      self.ser = serial.Serial(strPort, 115200)

      self.ax = deque([0.0]*maxLen)
      self.ay = deque([0.0]*maxLen)
      self.maxLen = maxLen

  # add to buffer
  def addToBuf(self, buf, val):
      if len(buf) < self.maxLen:
          buf.append(val)
      else:
          buf.pop()
          buf.appendleft(val)

  # add data
  def add(self, data):
      assert(len(data) == 2)
      self.addToBuf(self.ax, data[0])
      self.addToBuf(self.ay, data[1])

  # update plot
  def update(self, frameNum, a0, a1):
      try:
          line = self.ser.readline()
          # print(str(line))
          data = [float(val) for val in line.split()]
          
          print(str(data))
          # print data
          if(len(data) == 2):
              self.add(data)
              a0.set_data(range(self.maxLen), self.ax)
              a1.set_data(range(self.maxLen), self.ay)
      except KeyboardInterrupt:
          print('exiting')
      
      return a0, 

  # clean up
  def close(self):
      # close serial
      self.ser.flush()
      self.ser.close()    

# main() function
def main():
  numvalues = 15000


  # create parser
  parser = argparse.ArgumentParser(description="serial plotter")
  # add expected arguments
  parser.add_argument('--port', dest='port', required=True)

  # parse args
  args = parser.parse_args()
  
  # strPort = '/dev/tty.usbserial-A7006Yqh'
  strPort = args.port

  print('reading from serial port %s...' % strPort)

  # plot parameters
  # analogPlot = AnalogPlot(strPort, 100)

  print('plotting data...')
  
  ser = serial.Serial(strPort, 115200)
  # while True:
    # print(ser.readline())
  
  buffer = []
  pres = []
  mag = []
  pressure = np.empty([numvalues])
  magnitude = np.empty([numvalues])
  for i in range(numvalues):
    print("on number " +str(i))
    serline = ser.readline()
    print(str(serline))
    data = [float(val) for val in serline.split()]
    if(len(data) == 3):
        pres.append(data[0])
        mag.append(data[1])
        pressure[i-1] = data[0]
        magnitude[i-1] = data[1]
        buffer.append((data[0], data[1]))
    
  # print(str(buffer))
  
  x = np.arange(0, numvalues, 1)
  
  
  plt.subplot(2, 1, 1)
  plt.plot(mag)
  plt.title('Pressure and Impedance over Time')
  plt.ylabel('Impedance Magnitude')
  # plt.set_ylim(ymin=800, ymax = 850)
  plt.autoscale(enable=True, axis = 'both', tight = None)
  plt.subplot(2, 1, 2)
  plt.plot(pres)
  plt.ylabel('Pressure')
  plt.xlabel('sample number')
  plt.autoscale(enable=True, axis = 'both', tight = None)
  plt.show()

  
  # y = np.array(
  
  # set up animation
  # fig = plt.figure()
  # ax = plt.axes(xlim=(0, 100), ylim=(900, 910))
  # a0, = ax.plot([], [])
  # a1, = ax.plot([], [])
  # anim = animation.FuncAnimation(fig, analogPlot.update, 
                                 # fargs=(a0, a1), 
                                 # interval=30)

  # show plot
  # plt.show()
  
  # clean up
  # analogPlot.close()

  print('exiting.')
  

# call main
if __name__ == '__main__':
  main()