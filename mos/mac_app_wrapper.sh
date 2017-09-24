#!/bin/sh

EXEDIR=`dirname $0`
export DYLD_LIBRARY_PATH=`dirname $0`
`dirname $0`/_`basename $0`
