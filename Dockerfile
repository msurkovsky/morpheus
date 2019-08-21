FROM ubuntu:bionic

RUN apt-get update && apt-get install -y --no-install-recommends \
  llvm-8 llvm-8-tools llvm-8-dev clang-8 build-essential cmake libmpich-dev \
  python3 python3-pip python3-setuptools python3-wheel vim

RUN pip3 install plumbum click

WORKDIR /morpheus
COPY . .

ENV PATH=/usr/lib/llvm-8/bin:${PATH}

RUN mkdir build && \
    cd build && \
    cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release .. && \
    make -j

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# should be evaluated with llvm-config-8 --bindir
ENV PATH=/usr/lib/llvm-8/bin:${PATH}
ENV LD_LIBRARY_PATH=/morpheus/build/libs-bin

ENTRYPOINT ["python3", "src/morpheus.py"]
