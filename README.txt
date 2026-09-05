To Run Our TaskForge:

1. cd cos214-prac4
2. open DockerDesktop
3. delete previous containers associated with taskforge
4. run command :    docker build -t taskforge .
5. run command :    docker run -it taskforge
6. run command :    docker run -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined taskforge gdb ./taskforge
please note that step 5 and 6 are interchangable as one will use gdb and one will run through entire program at once