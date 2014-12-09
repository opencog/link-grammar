#
# Dockerfile for the basic link-grammar source download.
# No compilation is performed.
#
FROM gcc:latest
MAINTAINER linasvepstas@gmail.com

RUN apt-get update
RUN apt-get -y autoremove
RUN apt-get -y install apt-utils

# Need wget to download link-grammar source
RUN apt-get install wget

# Need the locales for utf8
RUN apt-get install locales
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
RUN echo "ru_RU.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "he_IL.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "de_DE.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "lt_LT.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "tr_TR.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen 

# Now, download and build.
ADD download-latest.sh /
RUN chmod +x download-latest.sh
RUN /download-latest.sh
