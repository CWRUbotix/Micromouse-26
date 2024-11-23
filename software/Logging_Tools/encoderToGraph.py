import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

df = pd.read_csv('Logs/arduino_data.csv', header=0, delimiter = ',')
plt.rcParams["figure.autolayout"] = True

figure, axis = plt.subplots(1, 5)

# Create left/right arr for wheel ratios
leftOverRight = (df["Left Wheel Speed"]/df["Right Wheel Speed"])
leftOverRight = leftOverRight.replace([float('inf'), float('-inf')], float('nan')).fillna(0)
average = ((df["Left Wheel Speed"] + df["Right Wheel Speed"]) / 2)
max_vel = {}
# print(leftOverRight.replace([float('inf'), float('-inf')], float('nan')).fillna(0))

for subplot, wheel_str in enumerate(["Left Wheel Speed", "Right Wheel Speed", "Average Speed", "Wheel Ratio"]):# , "Average Speed"]):
    speeds = []
    for dataframe, voltage in [(df[df['Analog Value'] == voltage], voltage) for voltage in np.unique(df['Analog Value'])]: # Makes a dataframe for each analog value in the csv
        speed = dataframe[wheel_str] if subplot < 2 else \
            leftOverRight[df['Analog Value'] == voltage] if wheel_str == "Wheel Ratio" else \
            average[df['Analog Value'] == voltage]
        times = dataframe['Time'] / 1000.0   # Time in ms -> to seconds

        axis[subplot].plot(times,speed, label=voltage)
        axis[subplot].set_xlabel('time (seconds)')
        axis[subplot].set_ylabel('speed (m/s)')
        axis[subplot].set_xbound(0,3)
        axis[subplot].set_ybound(0,max(speed) * 1.5)
        axis[subplot].set_title(wheel_str)
        axis[subplot].legend(loc='upper right')
        speeds.append(max(speed))
    if wheel_str != "Wheel Ratio":
        max_vel[wheel_str] = speeds

for index, wheel_str in enumerate(["Left Wheel Speed", "Right Wheel Speed", "Average Speed"]):
    axis[4].plot(range(len(max_vel[wheel_str])),max_vel[wheel_str], label=wheel_str)
    axis[4].set_xlabel('time (seconds)')
    axis[4].set_ylabel('speed (m/s)')
    # axis[4].set_xbound(0,4)
    # axis[4].set_ybound(0,max(speed) * 1.5)
    axis[4].set_title(wheel_str)
    axis[4].legend(loc='upper right')

plt.show()