FROM phusion/baseimage:0.9.19
MAINTAINER xCash <xcash@xcash.com>

ENV HOME /root
ENV TERM xterm
CMD ["/sbin/my_init"]

# set apt source.list
COPY sources.list.aliyun /etc/apt/sources.list

# install dependency pkgs
RUN apt-get update && apt-get install -y \
    build-essential pkg-config libc6-dev m4 g++-multilib \
    autoconf libtool ncurses-dev unzip git python \
    zlib1g-dev wget bsdmainutils automake curl wget libzmq3-dev

# use /root
WORKDIR /root/

# clone and build dcash
RUN git clone https://github.com/60E/zcash
RUN cd zcash && git checkout docker && ./zcutil/build.sh

# setup zcash.conf
RUN mkdir .zcash
COPY dcash.conf /root/.zcash/zcash.conf

# logrotate
COPY logrotate-zcashd /etc/logrotate.d/zcashd

# add to service
RUN mkdir /etc/service/zcashd
COPY run /etc/service/zcashd/run
RUN chmod +x /etc/service/zcashd/run

# download keys
RUN  cd /root \
     && wget https://antpool.com/zcash-params.tar.gz \
     && tar zxvf zcash-params.tar.gz \
     && rm zcash-params.tar.gz \
     && mv zcash-params .zcash-params

# cleanup
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# rpc port
EXPOSE 18232

# zcash port
EXPOSE 18234
