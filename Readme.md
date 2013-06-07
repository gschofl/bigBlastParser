# _BlastParser_

_blastParser_ is a very fast (SAX-style) NCBI BLAST XML parser for very large BLAST XML files.


It will parse the XML file into a SQLite database, containing three tables __query__, __hit__,
__hsp__.

The maximum number of hits parsed per query, and the maximum number of hsps parsed per hit
are controlled by command line options.

== Install

You will need the Xerces-C++ XML parser and SQLite. On Ubuntu use

  	apt-get install libxerces-c-dev libsqlite3-dev

then download and build the program:

	git clone git://github.com/gschofl/BigBlastParser
	cd BigBlastParser
	make
	make clean

== Command line usage

=== Usage

  bigBlastParser [options] <blastfile>.xml

    -o, --out 	dbName        Output SQLite database (default: <blastfile>.db)
    --max_hit	n        	  Maximum number of hits parsed from a query (default: 20);
    						  (set -1 to parse all available hits)
    --max_hit	n 		      Maximum number of hsps parsed from a hit (default: 20);
    						  (set -1 to parse all available hsps)
    --reset_at  n 	 		  After <n> queries are parsed, the data is dumped to the
                              database file before parsing is resumed. This helps to
                              keep the memory footprint small (default: 1000)
    -h, --help                show help

