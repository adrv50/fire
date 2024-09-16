FROM python:3.12-slim

RUN apt-get update && apt-get install -y clang

WORKDIR /flame

COPY include/   include/
COPY src/       src/
COPY build.py   .
COPY build.sh   .
COPY build.json .

RUN ./build.sh --install

WORKDIR /