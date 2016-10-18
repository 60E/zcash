FROM phusion/baseimage:0.9.19
MAINTAINER xCash <xcash@xcash.com>

ENV HOME /root
ENV TERM xterm
CMD ["/sbin/my_init"]

ADD sources.list.aliyun /etc/apt/sources.list

RUN apt-get update && \
    apt-get install -y build-essential pkg-config libc6-dev m4 g++-multilib \
    	    autoconf libtool ncurses-dev unzip git python \
	    zlib1g-dev wget bsdmainutils automake curl wget libzmq3-dev

RUN cd /root && git clone https://github.com/60E/zcash
RUN cd /root/zcash && git checkout 60E && ./zcutil/build.sh

RUN mkdir /root/.zcash

ADD xcash.conf /root/.zcash/zcash.conf

ADD logrotate-zcashd /etc/logrotate.d/zcashd

RUN mkdir /etc/service/zcashd
ADD run /etc/service/zcashd/run
RUN chmod +x /etc/service/zcashd/run

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 8234 18234 18445





