FROM ubuntu:focal

RUN apt-get update
RUN apt-get install -y curl make build-essential python3

WORKDIR /app/motorshades
ENV PATH="/app/motorshades/bin:${PATH}"

COPY Makefile ./

RUN make install_arduino_cli
RUN make deps

COPY motorshades.ino ./
COPY Secrets.h ./
