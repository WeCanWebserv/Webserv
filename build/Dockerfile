FROM gcc:10

RUN ln -sf /usr/share/zoneinfo/Asia/Seoul /etc/localtime

RUN set -ex; \
    apt-get update; \
    apt-get install -y \
# Programming
    cppcheck \
    cflow \
    clang \
    clang-format \
    clang-tidy \
    gdb \
    gcovr \
    google-perftools \
    python3 \
# Version Control
    git \
# Tools
    vim \
    python3-pip \
    dnsutils \
    finger \
    netcat \
    tcpdump \
    telnet \
    net-tools \
    iputils-ping \
    traceroute \
    whois \
    curl \
    sudo \
    wget \
    man-db \
    manpages-dev \
    locales \
    tldr \
    zsh \
# Remote
    openssh-server;

#Adds a locale to a debian system in non-interactive mode

RUN set-ex; \
    sed -i '/^#.* en_US.* /s/^#//' /etc/locale.gen; \
    sed -i '/^#.* ko_KR.* /s/^#//' /etc/locale.gen; \
    locale-gen;

RUN curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo gpg --dearmor -o /usr/share/keyrings/githubcli-archive-keyring.gpg \
    "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null \
    apt update \
    apt install gh;

# RUN sed -ri 's/UsePAM yes/#UsePAM yes/g' /etc/ssh/sshd_config
RUN set -ex; \
    echo 'root:pass' | chpasswd; \
    mkdir -p /var/run/sshd; \
    mkdir -p /root/.ssh;

RUN yes | sh -c "$(wget https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh -O -)"

ADD ./entrypoint.sh /usr/local/bin/
ADD ./sshd_config /etc/ssh/
RUN chmod 0755 /usr/local/bin/entrypoint.sh

ENTRYPOINT [ "entrypoint.sh" ]
