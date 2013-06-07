#ifndef BLASTSAXHANDLER_HPP
#define BLASTSAXHANDLER_HPP

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>

#include "Blast.hpp"
#include "XercesString.hpp"

using namespace xercesc;

// Callbacks that receive character data and
// notification about the beginning and end of
// elements.
class BlastQueryContentHandler : public DefaultHandler
{
    public:
        BlastQueryContentHandler(std::vector<BlastQuery>& queryList,
                                 std::string dbName = "blast.db",
                                 int max_hit = -1,
                                 int max_hsp = -1,
                                 int reset_at = 1000)
            : query_list_(queryList),
              db_(dbName, BLAST_DB_SCHEMA),
              max_hit_(max_hit),
              max_hsp_(max_hsp),
              reset_at_(reset_at)
        { }

        void startDocument();

        void endDocument();

        void startElement(
                const XMLCh * const uri,
                const XMLCh * const localname,
                const XMLCh * const qname,
                const Attributes &attrs);

        void endElement(
                const XMLCh * const uri,
                const XMLCh * const localname,
                const XMLCh * const qname);

        void characters(const XMLCh * const chars, const XMLSize_t length);

        void fatalError(const SAXParseException &exc);

        static void printState();

    protected:
        void dump_to_sqliteDB();
        std::vector<BlastQuery>&            query_list_;
        std::vector<BlastHit>               hit_list_;
        std::vector<Hsp>                    hsp_list_;
        XercesString                        currText_;

        // SQLite database
        SqliteDB db_;

        // General Tags
        XercesString iteration = fromNative ("Iteration");
        XercesString iterationHits = fromNative ("Iteration_hits");
        XercesString hit = fromNative ("Hit");
        XercesString hitHsps = fromNative ("Hit_hsps");
        XercesString hsp = fromNative ("Hsp");

        // Query Tags
        XercesString queryId = fromNative ("Iteration_iter-num");
        XercesString queryDef = fromNative ("Iteration_query-def");
        XercesString queryLen = fromNative ("Iteration_query-len");

        // Hit Tags
        XercesString hitNum = fromNative ("Hit_num");
        XercesString hitId = fromNative ("Hit_id");
        XercesString hitDef = fromNative ("Hit_def");
        XercesString hitAccn = fromNative ("Hit_accession");
        XercesString hitLen = fromNative ("Hit_len");

        // Hsp Tags
        XercesString hspNum = fromNative ("Hsp_num");
        XercesString bitscore = fromNative ("Hsp_bit-score");
        XercesString score = fromNative ("Hsp_score");
        XercesString evalue = fromNative ("Hsp_evalue");
        XercesString queryFrom = fromNative ("Hsp_query-from");
        XercesString queryTo = fromNative ("Hsp_query-to");
        XercesString hitFrom = fromNative ("Hsp_hit-from");
        XercesString hitTo = fromNative ("Hsp_hit-to");
        XercesString queryFrame = fromNative ("Hsp_query-frame");
        XercesString hitFrame = fromNative ("Hsp_hit-frame");
        XercesString identity = fromNative ("Hsp_identity");
        XercesString positive = fromNative ("Hsp_positive");
        XercesString gaps = fromNative ("Hsp_gaps");
        XercesString alignLen = fromNative ("Hsp_align-len");
        XercesString qseq = fromNative ("Hsp_qseq");
        XercesString hseq = fromNative ("Hsp_hseq");
        XercesString midline = fromNative ("Hsp_midline");

        // counters
        unsigned int queryCounter_ = 0;
        unsigned int hitCounter_ = 0;
        unsigned int hspCounter_ = 0;

        // max number of hits and hsps to be parsed
        int max_hit_;
        int max_hsp_;
        int reset_at_;

        // states
        static bool inside_query_;
        static bool inside_hit_;
        static bool inside_hsp_;
        static bool skip_hit_;
        static bool skip_hsp_;

        // container for the currently parsed query, hit, and hsp instances
        static BlastQuery query_;
        static BlastHit hit_;
        static Hsp hsp_;
} ;


#endif // BLASTSAXHANDLER_HPP
