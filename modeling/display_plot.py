import argparse
import serial
from os.path import exists
from matplotlib import pyplot as plt
from matplotlib import animation
from dataclasses import dataclass

fig, fig_a = plt.subplots()
fig_b = fig_a.twinx()


@dataclass
class CurrentValues:
    temp_celsius: list[float]
    duty_cycle: list[float]
    seconds_accumulated: list[int]


def init_serial(serial_port):
    if not exists(serial_port):
        raise serial.SerialException(f"{serial_port} does not exist!")

    return serial.Serial(serial_port)


def animate_plot(i, current_values, serial_obj):
    data_input = serial_obj.read_all().decode("utf-8").lower().splitlines()
    for line in data_input:
        if "current boiler" in line:
            boiler_temp = float(line.split(": ")[1].strip())
            print(boiler_temp)
            current_values.temp_celsius.append(boiler_temp)
            current_values.seconds_accumulated.append(current_values.seconds_accumulated[-1] + 1)  # this is lazy
        if "duty cycle" in line:
            duty_cycle = float(line.split(": ")[1].strip())
            print(duty_cycle)
            current_values.duty_cycle.append(duty_cycle)

    fig_a.clear()
    fig_b.clear()
    fig_a.plot(current_values.seconds_accumulated[-60:], current_values.temp_celsius[-60:], 'r-', label='TempCelsius')
    fig_a.legend()
    fig_b.plot(current_values.seconds_accumulated[-60:], current_values.duty_cycle[-60:], 'b-', label='DutyCycle')
    fig_b.legend()
    plt.title("Boiler Temp and Duty Cycle over Time")


def show_plot(serial_obj):
    current_values = CurrentValues
    current_values.temp_celsius = [0.0]
    current_values.duty_cycle = [0.0]
    current_values.seconds_accumulated = [0]

    anim = animation.FuncAnimation(fig,
                                   animate_plot,
                                   cache_frame_data=False,
                                   fargs=(current_values, serial_obj))
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--port",
        action="store"
    )
    args = parser.parse_args()
    guisep_pi = init_serial(args.port)
    show_plot(guisep_pi)
