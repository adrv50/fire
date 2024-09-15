FROM debian:bookworm

RUN apt-get update && apt-get install -y clang make

WORKDIR /flame

COPY include/   include/
COPY src/       src/
COPY build.py   .
COPY build.sh   .
COPY build.json .

RUN ./build.sh --install

WORKDIR /
