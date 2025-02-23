import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

df = pd.read_csv('Logs/voltage_data.csv', header=0, delimiter = ',')
plt.rcParams["figure.autolayout"] = True
figure, axis = plt.subplots(1,2)
speedLeft = []
speedRight = []
volt = []

voltage = df["Voltage"][0].item()
temp = df[(df["Time"]>df.tail(1)["Time"].sum().item()/2) & (df["Voltage"]==voltage)]

while(len(temp)>0):
    volt.append(voltage)
    speedLeft.append(temp["Left Wheel Speed"].sum().item()/len(temp))
    speedRight.append(temp["Right Wheel Speed"].sum().item()/len(temp))
    voltage = df[df["Voltage"]>voltage]
    if len(voltage) == 0:
        break
    voltage = voltage.head(1)["Voltage"].item()
    temp = df[(df["Time"]>df.tail(1)["Time"].sum().item()/2) & (df["Voltage"]==voltage)]

axis[0].plot(volt, speedRight, color='r', label='Right Speed')
#axis[0].plot([1,6.5],[speedRight[0],speedRight[len(speedRight)-1]],color = 'g', label='Est Right Speed')

axis[1].plot(volt, speedLeft, color='r', label='Left Speed')
#axis[1].plot([1,6.5],[speedLeft[0],speedLeft[len(speedLeft)-1]],color = 'g', label='Est Left Speed')

plt.xlabel("Voltage")
plt.ylabel("Speed (m/s)")

axis[0].legend()
axis[1].legend()

print("Slope Left: "+str((speedLeft[0]-speedLeft[len(speedLeft)-1])/-5.5))
print("Slope Right: "+str((speedRight[0]-speedRight[len(speedRight)-1])/-5.5))

plt.show()
