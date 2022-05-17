FROM coc0a25/ns-3:base

COPY ./scratch /root/ns-allinone-3.29/ns-3.29/scratch
COPY ./src /root/ns-allinone-3.29/ns-3.29/src

WORKDIR /root/ns-allinone-3.29/ns-3.29
RUN ./waf build