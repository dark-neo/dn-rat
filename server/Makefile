
###########################################
# Copyright (c) 2014 dark_neo             #
# All rights reserved.                    #
#                                         #
# Source code released under BSD license. #
# See LICENSE file for more details.      #
###########################################

# C development
# NOTE: USE ALWAYS `cc` FOR COMPILER COMMAND. CROSS-COMPILER.
#	(for example, between Linux and *BSD systems)

#################################################
# 		C RULES									#
#												#
# CC 		== C compiler						#
# CFLAGS	== C compiler flags					#
# LDFLAGS	== C libraries path					#
# SRCS		== always .c						#
#################################################

PROJECT_NAME	= SCH-RAT (server)
CC				= cc
CFLAGS			= -std=c11 -Wall -O3 -g -I$(INCLUDES)
INCLUDES		= .
LDFLAGS			=
SRCS			= main.c
OBJS			= $(SRCS:.c=.o)
DELCOM			= rm -rf
EXEC			= server
VERSION			= 0.0.10pre-alpha
PREFIX			= /usr/local		# UNIX installation dir

default: help

help:
	@echo
	@echo "	$(PROJECT_NAME)"
	@echo
	@echo " Sergio Cruz"
	@echo " Copyright (c) 2014. All rights reserved."
	@echo
	@echo
	@echo " -- BUILD OPTIONS --"
	@echo
	@echo " help:		show this text and finish"
	@echo " all: 		build source code"
	@echo " depends:	build dependencies"
	@echo " clean:		delete object files"
	@echo " distclean:	delete object and executable files"
	@echo

all: depends $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS) $(LDFLAGS)
	@echo
	@echo
	@echo "Program version: $(VERSION)"
	@echo

# Generic Makefile part.
# It can be used to build any executable just by changing the definitions
# above (mainly, C(XX)FALGS, LD(XX)FLAGS, SRCS, OBJS, DELCOM and EXEC variables)
# and deleting dependencies appended to the file from 'make depend'.

# This is a suffix replacement rule for building .o's from .c's.
# It uses automatic variables:
#	$<: the name of the prerequisite of the rule (a .c file)
#	$@: the name of the target of the rule (a .o file)
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(DELCOM) $(OBJS)

distclean: clean
	$(DELCOM) $(EXEC)

depends: $(SRC)
	$(CC) $(CFLAGS) -E -M $(SRCS) > depends

