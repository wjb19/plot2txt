FROM ubuntu:20.04

RUN apt-get update && \
   DEBIAN_FRONTEND=noninteractive \ 
	 apt-get install -y libopencv-dev \
   poppler-utils libboost-all-dev bc

RUN mkdir -p /tmp/share/tessdata
COPY tessdata /tmp/share/tessdata
COPY binaries.tgz /opt/
ENV PATH $PATH:/opt/bin
ENV LD_LIBRARY_PATH /opt/bin
RUN cd /opt/ && tar -xvf binaries.tgz && rm *tgz
ADD create_es_doc.sh /opt/bin


