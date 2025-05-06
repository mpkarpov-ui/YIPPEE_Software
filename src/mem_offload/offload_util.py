import subprocess
from pathlib import Path
import os
import serial
import time

current_root = Path.cwd()
os.chdir(Path(__file__).absolute().parent.parent.parent)

subprocess.run(["platformio", "run", "-e", "mem_offload", "-t", "upload"])

print("Listing connected devices:")
subprocess.run(["platformio", "device", "list"])

port = input("Choose a port name from the above list: ")
outfile = None
DO_PRINTS = True

with serial.Serial(port, 9600) as ser:
    print("Mem offload helper -- Type 'help' for commands")
    while True:
        command = input(">> ").strip() + "\n"
        DO_PRINTS = True
        if(command.upper().startswith("DUMP")):
            # It's a read command so we pipe the output to a file
            DO_PRINTS = False
            if command.upper() == "DUMP\n":
                fname = time.time_ns() // 1000
                outfile = open(f"./mem_dumps/dump{fname}.bin", "w")
                print(f"Writing full dump to dump{fname}.bin")
            else:
                flight_to_dump = command[4]
                if (flight_to_dump == "1" or flight_to_dump == "0"):
                    filename = command.split(" ")[1].strip()
                    command = "r" + flight_to_dump + "\n"
                    outfile = open(f"./mem_dumps/{filename}", "w")
                    print(f"Writing to {filename}") 
                else:
                    print("\033[31mInvalid flight number\033[0m")
        else:
            outfile = None


        ser.write(command.upper().encode("utf-8"))

        # Give the board some time to respond
        NEXT_TIME = time.time() + 0.2
        while time.time() < NEXT_TIME:
            if(ser.in_waiting > 0):
                break

        CMD_TIMEOUT = time.time() + 0.5
        cmd_is_done = False
        while True:
            if(ser.in_waiting == 0):

                if time.time() > CMD_TIMEOUT:

                    if outfile:
                        outfile.close()
                        print(f"Finished writing. File closed.")

                    if not cmd_is_done:
                        print("\033[33mNo response from board\033[0m")

                    break

                continue

            CMD_TIMEOUT = time.time() + 0.5
            line = ser.readline().decode("utf-8").strip()

            if line == "<DONE>":
                print("\033[32mDone\033[0m")
                cmd_is_done = True
                continue

            if line.startswith("#"):
                com = line[2:]
                print("\033[34m" + com + "\033[0m")
                
            else:
                if DO_PRINTS:
                    print(line)
                if outfile:
                    outfile.write(line + "\n")

