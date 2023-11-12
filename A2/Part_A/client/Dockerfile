# Use a base image with a suitable Linux distribution
FROM gcc:latest

# Install necessary dependencies
COPY client.c /client/

# Set the working directory
WORKDIR /client/

RUN gcc -o client client.c

# Specify the command to run when the container starts
CMD ["./client"]