#!/bin/sh

pkgconf "$@" | tr -s ' \t' '\n'
