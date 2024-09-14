FROM debian:bookworm

RUN apt-get update && apt-get install -y clang make

WORKDIR /flame

COPY include/ include/
COPY src/     src/
COPY Makefile .

RUN make install -j

WORKDIR /
