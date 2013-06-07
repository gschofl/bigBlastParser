# _bigBlastParser_

_bigBlastParser_ is a very fast (SAX-style) NCBI BLAST parser for very large BLAST XML files.


It will parse the BLAST data into a SQLite database, generating three tables __query__, __hit__,
__hsp__, that can be queried using standard SQL.


The tables are designed as follows:

        CREATE TABLE query(
                query_id      INTEGER,
                query_def     TEXT,
                query_len     INTEGER,
                PRIMARY KEY (query_id)
                );
        CREATE INDEX Fquery ON query (query_id);

        CREATE TABLE hit(
                query_id      INTEGER,
                hit_id        INTEGER,
                hit_num       INTEGER,
                gene_id       TEXT,
                accession     TEXT,
                definition    TEXT,
                length        INTEGER,
                PRIMARY KEY (hit_id),
                FOREIGN KEY (query_id) REFERENCES query (query_id)
                );
        CREATE INDEX Fhit ON hit (hit_id);
        CREATE INDEX Fhit_hit_query ON hit (query_id, hit_id);
        CREATE INDEX Fhit_query ON hit (query_id);

        CREATE TABLE hsp(
                query_id      INTEGER,
                hit_id        INTEGER,
                hsp_id        INTEGER,
                hsp_num       INTEGER,
                bit_score     FLOAT,
                score         INTEGER,
                evalue        FLOAT,
                query_from    INTEGER,
                query_to      INTEGER,
                hit_from      INTEGER,
                hit_to        INTEGER,
                query_frame   INTEGER,
                hit_frame     INTEGER,
                identity      INTEGER,
                positive      INTEGER,
                gaps          INTEGER,
                align_len     INTEGER,
                qseq          TEXT,
                hseq          TEXT,
                midline       TEXT,
                PRIMARY KEY (hsp_id),
                FOREIGN KEY (hit_id) REFERENCES hit (hit_id)
                FOREIGN KEY (query_id) REFERENCES query (query_id)
                );
        CREATE INDEX Fhsp ON hsp (hsp_id);
        CREATE INDEX Fhsp_hit ON hsp (hit_id);
        CREATE INDEX Fhsp_hit_query ON hsp (query_id, hit_id, hsp_id);
        CREATE INDEX Fhsp_query ON hsp (query_id);

The maximum number of hits parsed per query, and the maximum number of hsps parsed per hit
are controlled by command line options.

## Install

You will need the Xerces-C++ XML parser and SQLite. On Ubuntu use

  	apt-get install libxerces-c-dev libsqlite3-dev

then download and build the program:

    git clone https://github.com/gschofl/BigBlastParser.gits
	cd BigBlastParser
	make
	make clean

## Command line usage

### Usage

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

