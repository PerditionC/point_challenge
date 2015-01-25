# point_search: #
    Example implementation of main point_search program.

## Overview: ##
	See main readme for an introduction to competion.  This is source to a clone of the main program.  It can be used for development purposes (e.g. tracing your challenger, testing with specific inputs, etc.) including on a 32bit version of Windows (Windows XP+ should work, but only tested on Windows 7).

## Usage: ##
*(Based on original program.  -sX is not supported.)*

    Description: Given [point count] ranked points on a plane, find the [result count] most important points inside [query count] rectangles.  You can specify a list of plugins that solve this problem, and their results and performance will be compared!
    	Usage:
    		point_search.exe plugin_paths [-pN] [-qN] [-rN] [-s]
    	Options:
 		   -pN: point count (default: %u)
 		   -qN: query count (default: %u)
 		   -rN: result count (default: %u)
 		   -sX: specify seed (default: random)

    Example:
	    point_search.exe reference.dll coyote.dll -p10000000 -q100000 -r20 