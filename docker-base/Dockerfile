#
# Dockerfile for the basic link-grammar source download.
# No compilation is performed.
#
FROM ubuntu:14.04
MAINTAINER linasvepstas@gmail.com

RUN apt-get update
RUN apt-get -y install apt-utils
RUN apt-get -y install gcc g++ make

# Need wget to download link-grammar source
RUN apt-get -y install wget

# Download the current released version of link-grammar.
# RUN http://www.abisource.com/downloads/link-grammar/current/link-grammar-5*.tar.gz
# The wget tries to guess the correct file to download w/ wildcard
RUN wget -r --no-parent -nH --cut-dirs=2 http://www.abisource.com/downloads/link-grammar/current/

# Unpack the sources, too.
RUN tar -zxf current/link-grammar-5*.tar.gz

# Need the locales for utf8
RUN apt-get install locales
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
RUN echo "ru_RU.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "he_IL.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "de_DE.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "lt_LT.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "fa_IR.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "ar_AE.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "tr_TR.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen
