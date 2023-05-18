import argparse
import serial
from os.path import exists
from matplotlib import pyplot as plt
from matplotlib import animation
from dataclasses import dataclass


@dataclass
class CurrentValues:
    temp_celsius: list[float]
    duty_cycle: list[float]
    seconds_accumulated: int


def init_serial(serial_port):
    if not exists(serial_port):
        raise serial.SerialException(f"{serial_port} does not exist!")

    return serial.Serial(serial_port)


def show_plot(serial_obj):
    current_values = CurrentValues
    current_values.temp_celsius = [0.0]
    current_values.duty_cycle = [0.0]
    current_values.seconds_accumulated = 0

    fig = plt.figure()
    fig_a = fig.add_subplot(1, 1, 1)
    fig_b = fig.add_subplot(1, 1, 1)

    def animate_plot(i):
        fig_a.clear()
        fig_b.clear()
        fig_a.plot(current_values.seconds_accumulated[-60:], current_values.temp_celsius[-60:])
        fig_b.plot(current_values.seconds_accumulated[-60:], current_values.duty_cycle[-60:])
        plt.xticks()
        plt.title("Boiler Temp and Duty Cycle over Time")

    animation.FuncAnimation(fig, animate_plot)
    plt.show()

    while True:
        data_input = serial_obj.read_all().decode("utf-8").lower().splitlines()
        for line in data_input:
            if "current boiler" in line:
                current_values.temp_celsius.append(float(line.split(": ")[1].strip()))
            if "duty cycle" in line:
                current_values.duty_cycle.append(float(line.split(": ")[1].strip()))
        time.sleep(1)
        current_values.seconds_accumulated += 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--port",
        action="store"
    )
    args = parser.parse_args()
    guisep_pi = init_serial(args.port)
    show_plot(guisep_pi)
