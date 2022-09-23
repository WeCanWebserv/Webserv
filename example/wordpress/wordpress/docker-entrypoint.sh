#!/bin/bash

MYSQL_HOST=db:3306
MYSQL_DATABASE=wordpress
MYSQL_USER=webserv
MYSQL_PASSWORD=webserv123

DOMAIN_NAME=localhost:8080

WP_TITLE=we-can-webserv

WP_ADMIN=webserv
WP_ADMIN_PASSWORD=webserv
WP_ADMIN_EMAIL=webserv@example.com

if [ ! -e /var/www/wordpress/wp-config.php ]; then
  echo "** creating wp-config file"
  wp config create \
    --allow-root \
    --path=/var/www/wordpress \
    --dbname=${MYSQL_DATABASE} \
    --dbuser=${MYSQL_USER} \
    --dbpass=${MYSQL_PASSWORD} \
    --dbhost=${MYSQL_HOST} \
    || exit 1

  echo "** wp core install.."
  wp core install \
    --allow-root \
    --path=/var/www/wordpress \
    --skip-email \
    --url=${DOMAIN_NAME} \
    --title=${WP_TITLE} \
    --admin_user=${WP_ADMIN} \
    --admin_password=${WP_ADMIN_PASSWORD} \
    --admin_email=${WP_ADMIN_EMAIL}
fi

make re
exec "$@"
