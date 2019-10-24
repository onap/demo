FROM registry.access.redhat.com/ubi7/ubi-minimal:latest

ENV OPERATOR=/usr/local/bin/prometheus-remote-config-operator \
    USER_UID=1001 \
    USER_NAME=prometheus-remote-config-operator

# install operator binary
COPY build/_output/bin/prometheus-remote-config-operator ${OPERATOR}

COPY build/bin /usr/local/bin
RUN  /usr/local/bin/user_setup

ENTRYPOINT ["/usr/local/bin/entrypoint"]

USER ${USER_UID}
