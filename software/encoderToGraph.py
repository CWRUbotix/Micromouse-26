import matplotlib.pyplot as plt
import csv
arr = [[0,0,0,0,0]]
with open('encoder.csv', mode ='r')as file:
  csvFile = csv.reader(file)
  for lines in csvFile:
        arr.append(lines)
arr.pop(0)
arr.pop(0)
arr.pop(0)
arr.pop(0)
arr.pop(0)
arr.pop(0)
arr.pop(0)
cnt = 0
plt.rcParams["figure.autolayout"] = True
figure, axis = plt.subplots(3, 6)
axisX = 0
axisY = 0
while cnt < len(arr):
    left = []
    right = []
    leftOverRight = []
    y = []
    voltage = arr[cnt][4]

    while cnt < len (arr) and arr[cnt][4] == voltage :
        left.append(float(arr[cnt][1]))
        right.append(float(arr[cnt][3]))
        if float(arr[cnt][3])!=0 and float(arr[cnt][1])!=0:
            leftOverRight.append(float(arr[cnt][1])/float(arr[cnt][3]))
        else:
            leftOverRight.append(1)
        y.append(float(arr[cnt][5])/1000.00)
        cnt+=1
    axis[axisX,axisY].plot(y,left)
    axis[axisX,axisY].set_xlabel('time (seconds)')
    axis[axisX,axisY].set_ylabel('speed (m/s)')
    axis[axisX,axisY].set_xbound(0,3)
    axis[axisX,axisY].set_ybound(0,.82)
    axis[axisX,axisY].set_title(voltage + " left")
    axisY +=1
    if (axisY>=6):
        axisY=0
        axisX+=1
    axis[axisX,axisY].plot(y,right)
    axis[axisX,axisY].set_xlabel('time (seconds)')
    axis[axisX,axisY].set_ylabel('speed (m/s)')
    axis[axisX,axisY].set_xbound(0,3)
    axis[axisX,axisY].set_ybound(0,.82)
    axis[axisX,axisY].set_title(voltage + " right")
    axisY +=1
    if (axisY>=6):
        axisY=0
        axisX+=1
    axis[axisX,axisY].plot(y,leftOverRight)
    axis[axisX,axisY].set_xlabel('time (seconds)')
    axis[axisX,axisY].set_ylabel('Left speed / Right speed (ratio)')
    axis[axisX,axisY].set_xbound(0,3)
    axis[axisX,axisY].set_ybound(0,2)
    axis[axisX,axisY].set_title(voltage + " Left speed / Right speed")
    axisY +=1
    if (axisY>=6):
        axisY=0
        axisX+=1
plt.show()