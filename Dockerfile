FROM coc0a25/ns-3:base

RUN apt-get install -y libcrypto++6 libcrypto++6-dbg libcrypto++-dev
ADD ./src /root/ns-allinone-3.29/ns-3.29/scratch