ARG IDF_VER=release-v5.4
FROM espressif/idf:$IDF_VER
ENV DEBIAN_FRONTEND=noninteractive

ARG USER_ID=1000
ARG GROUP_ID=1000
ARG USER_NAME=espidf

RUN existing_user=$(getent passwd $USER_ID | cut -d: -f1) && \
    if [ -n "$existing_user" ]; then \
        usermod -l $USER_NAME -d /home/$USER_NAME -m $existing_user && \
        groupmod -n $USER_NAME $(getent group $GROUP_ID | cut -d: -f1) 2>/dev/null || true; \
    else \
        groupadd -g $GROUP_ID $USER_NAME 2>/dev/null || true && \
        useradd --create-home --home-dir /home/$USER_NAME --shell /bin/bash --uid $USER_ID --gid $GROUP_ID $USER_NAME; \
    fi && \
    usermod -a -G adm,sudo $USER_NAME 2>/dev/null || true && \
    echo $USER_NAME:$USER_NAME | chpasswd && \
    echo "$USER_NAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN apt-get update && apt-get install -y ruby

USER $USER_NAME

WORKDIR /project
ENTRYPOINT ["/opt/esp/entrypoint.sh"]
CMD ["/bin/bash"]
