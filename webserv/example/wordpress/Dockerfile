FROM debian:buster AS wordpress

RUN apt-get -y -qq update && \
	apt-get -y -qq install \
	php-mysql \
	php-mbstring \
	php-cgi \
	wget \
	curl \
	mariadb-client

WORKDIR /var/www/wordpress/

RUN adduser --gid 82 www-data www-data

RUN wget https://raw.githubusercontent.com/wp-cli/builds/gh-pages/phar/wp-cli.phar && \
	chmod +x wp-cli.phar && \
	mv wp-cli.phar /usr/local/bin/wp

# install wordpress.
RUN wp core download --locale=ko_KR --allow-root
RUN chown -R www-data:www-data /var/www/wordpress

# build & run webserv
FROM wordpress AS webserv

RUN apt-get -y -qq update && \
	apt-get -y -qq install \
	build-essential \
	procps \
	python3 \
	lsof \
	vim

COPY docker-entrypoint.sh /usr/local/bin/

WORKDIR /usr/share/webserv

EXPOSE 8080
EXPOSE 8888

# USER www-data

ENTRYPOINT [ "docker-entrypoint.sh" ]
# CMD [ "./webserv", "config/wordpress.conf" ]
CMD [ "tail", "-f", "/dev/null" ]
