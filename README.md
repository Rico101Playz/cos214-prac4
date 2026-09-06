# TaskForge

A hierarchical work-processing system for manufacturing, demonstrating
Iterator, Composite, State, and Decorator (COS 214 Practical 4).

DockerDesktop is needed to run the taskforge. This will have all the debugging inside it

from project's parent folder(in windows powershell):
docker build -t taskforge .
docker run -it taskforge


the following command takes you into the container to run commands(If you are using Windows powershell not wsl):
docker run -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -v "${PWD}:/app" taskforge bash

compile:
make clean && make

run: 
./taskforge



Debugging with GDB

Inside the container session:

gdb ./taskforge

Commands needed for GDB:
break ClassName::methodName : Pause execution when that function is about to run
step : Execute the next line, dives in any function call it makes
print someVariable : Show the current value of a variable
catch throw : Pauses the run when C++ exception is thrown

Inside the container session:
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./taskforge
Inside windows powershell:
docker run -it taskforge valgrind --leak-check=full --show-leak-kinds=all ./taskforge
