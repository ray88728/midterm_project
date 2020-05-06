import numpy as np

import serial

import time


waitTime = 0.1


# generate the waveform table



song =np.array(
[
  261, 261, 392, 392, 440, 440, 392,

  349, 349, 330, 330, 294, 294, 261,

  392, 392, 349, 349, 330, 330, 294,

  392, 392, 349, 349, 330, 330, 294,

  261, 261, 392, 392, 440, 440, 392,

  349, 349, 330, 330, 294, 294, 261
]

)

song = song /500
# output formatter

a = 1

formatter = lambda x: "%.3f" % x


# send the waveform table to K66F

serdev = '/dev/ttyACM0'

s = serial.Serial(serdev)

while a != 2:
    b = s.readline()
    c = float(b[0])
    if c > 0.5:
        print("Sending signal ...")

        #print("It may take about %d seconds ..." % (int(signalLength * waitTime)))


        for data in song:

            s.write(bytes(formatter(data), 'UTF-8'))

            time.sleep(waitTime)

        #s.close()

        print("Signal sended")
