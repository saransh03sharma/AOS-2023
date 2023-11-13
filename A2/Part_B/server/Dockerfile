# Use a base image with a suitable Linux distribution
FROM gcc:latest

# Install necessary dependencies
COPY server.c /server/

# Set the working directory
WORKDIR /server/

RUN gcc -o server server.c

# Specify the command to run when the container starts
CMD ["./server"]