FROM phusion/baseimage:0.9.19
MAINTAINER xCash <xcash@xcash.com>

ENV HOME /root
ENV TERM xterm
CMD ["/sbin/my_init"]

COPY sources.list.aliyun /etc/apt/sources.list

RUN apt-get update && apt-get install -y \
    build-essential pkg-config libc6-dev m4 g++-multilib \
    autoconf libtool ncurses-dev unzip git python \
    zlib1g-dev wget bsdmainutils automake curl wget libzmq3-dev

WORKDIR /root/

RUN git clone https://github.com/60E/zcash
RUN cd zcash && git checkout docker && ./zcutil/build.sh

RUN mkdir .zcash

COPY xcash.conf /root/.zcash/zcash.conf

COPY logrotate-zcashd /etc/logrotate.d/zcashd

RUN mkdir /etc/service/zcashd
COPY run /etc/service/zcashd/run
RUN chmod +x /etc/service/zcashd/run

RUN  cd /root \
     && wget https://antpool.com/zcash-params.tar.gz \
     && tar zxvf zcash-params.tar.gz \
     && rm zcash-params.tar.gz \
     && mv zcash-params .zcash-params

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 8234 18234 18445
