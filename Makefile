DC_FILE = -f ./docker-compose.yml
DC = docker-compose $(DC_FILE)

DCBUILD=$(DC) build
DCCREATE= $(DC) create
DCUP = $(DC) up
DCDOWN = $(DC) down

.PHONY: build
build:
	$(DCBUILD) $(c)

.PHONY: create
create:
	@$(DCCREATE) $(c)

.PHONY: up
up: build
	@$(DCUP) -d $(c)

.PHONY: down
down:
	@$(DCDOWN) $(c) --volume > /dev/null 2>&1

.PHONY: webserv
webserv:
	@$(DC) exec $@ bash

.PHONY: clean
clean: down
	@yes | docker image prune -a > /dev/null 2>&1

.PHONY: re
re: clean up