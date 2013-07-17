#ifndef BLAST_HPP
#define BLAST_HPP

#include "SQLite.hpp"

using std::cout;
using std::endl;

// High-scoring segment pairs
class Hsp
{
    public:
        typedef Table<Hsp>  HspTable;
        typedef Column<Hsp> HspColumn;

        // Default constructor of Hsp
        Hsp()
        {
            num_ = 0;
        }

        // Default destructor
        ~Hsp() { }

        // Construct an Hsp with the given properties
        Hsp( const int& num,
             const double& bit_score,
             const int& score,
             const double& evalue,
             const int& query_from,
             const int& query_to,
             const int& hit_from,
             const int& hit_to,
             const int& query_frame,
             const int& hit_frame,
             const int& identity,
             const int& positive,
             const int& gaps,
             const int& align_len,
             const std::string& qseq,
             const std::string& hseq,
             const std::string& midline,
             const int& count = 0,
             const int& hitID = 0,
             const int& queryID = 0)
            : num_(num),
              bit_score_(bit_score),
              score_(score),
              evalue_(evalue),
              query_from_(query_from),
              query_to_(query_to),
              hit_from_(hit_from),
              hit_to_(hit_to),
              query_frame_(query_frame),
              hit_frame_(hit_frame),
              identity_(identity),
              positive_(positive),
              gaps_(gaps),
              align_len_(align_len),
              qseq_(qseq),
              hseq_(hseq),
              midline_(midline),
              count_(count),
              hit_id_(hitID),
              query_id_(queryID)
        {
        }

        // Getters
        int getQueryID() const { return query_id_; }
        int getHitID() const { return hit_id_; }
        int getID() const { return count_; }
        int getHspNum() const { return num_; }
        double getBitScore() const { return bit_score_; }
        int getScore() const { return score_; }
        double getEvalue() const { return evalue_; }
        int getQueryFrom() const { return query_from_; }
        int getQueryTo() const { return query_to_; }
        int getHitFrom() const { return hit_from_; }
        int getHitTo() const { return hit_to_; }
        int getQueryFrame() const { return query_frame_; }
        int getHitFrame() const { return hit_frame_; }
        int getIdentity() const { return identity_; }
        int getPositive() const { return positive_; }
        int getGaps() const { return gaps_; }
        int getAlignLen() const { return align_len_; }
        std::string getQSeq() const { return qseq_; }
        std::string getHSeq() const { return hseq_; }
        std::string getMidline() const { return midline_; }

        // Setters
        void setQueryID( const int& queryID ) { query_id_ = queryID; }
        void setHitID( const int& hitID ) { hit_id_ = hitID; }
        void setID( const int& count ) { count_ = count; }
        void setHspNum( const int& num ) { num_ = num; }
        void setBitScore( const double& bit_score )  { bit_score_ = bit_score; }
        void setScore( const int& score )  { score_ = score ; }
        void setEvalue( const double& evalue )  { evalue_ = evalue; }
        void setQueryFrom( const int& query_from )  { query_from_ = query_from; }
        void setQueryTo( const int& query_to )  { query_to_ = query_to; }
        void setHitFrom( const int& hit_from )  { hit_from_ = hit_from; }
        void setHitTo( const int& hit_to )  { hit_to_ = hit_to; }
        void setQueryFrame( const int& query_frame )  { query_frame_ = query_frame; }
        void setHitFrame( const int& hit_frame )  { hit_frame_ = hit_frame; }
        void setIdentity( const int& identity )  { identity_ = identity; }
        void setPositive( const int& positive )  { positive_ = positive; }
        void setGaps( const int& gaps )  { gaps_ = gaps; }
        void setAlignLen( const int& align_len )  { align_len_ = align_len; }
        void setQSeq( const std::string& qseq )  { qseq_ = qseq; }
        void setHSeq( const std::string& hseq )  { hseq_ = hseq; }
        void setMidline( const std::string& midline )  { midline_ = midline; }

        // SQL Table
        static HspTable& table() {
        static HspTable tbl = HspTable::table("hsp",
                HspColumn("query_id",   makeAttr(&Hsp::query_id_)), // foreign key
                HspColumn("hit_id",     makeAttr(&Hsp::hit_id_)),   // foreign key
                HspColumn("hsp_id",     makeAttr(&Hsp::count_)),    // primary key
                HspColumn("hsp_num",    makeAttr(&Hsp::num_)),
                HspColumn("bit_score",  makeAttr(&Hsp::bit_score_)),
                HspColumn("score",      makeAttr(&Hsp::score_)),
                HspColumn("evalue",     makeAttr(&Hsp::evalue_)),
                HspColumn("query_from", makeAttr(&Hsp::query_from_)),
                HspColumn("query_to",   makeAttr(&Hsp::query_to_)),
                HspColumn("hit_from",   makeAttr(&Hsp::hit_from_)),
                HspColumn("hit_to",     makeAttr(&Hsp::hit_to_)),
                HspColumn("query_frame",makeAttr(&Hsp::query_frame_)),
                HspColumn("hit_frame",  makeAttr(&Hsp::hit_frame_)),
                HspColumn("identity",   makeAttr(&Hsp::identity_)),
                HspColumn("positive",   makeAttr(&Hsp::positive_)),
                HspColumn("gaps",       makeAttr(&Hsp::gaps_)),
                HspColumn("align_len",  makeAttr(&Hsp::align_len_)),
                HspColumn("qseq",       makeAttr(&Hsp::qseq_)),
                HspColumn("hseq",       makeAttr(&Hsp::hseq_)),
                HspColumn("midline",    makeAttr(&Hsp::midline_))
            );
            return tbl;
        }

        // operators
        friend std::ostream& operator<<(std::ostream& out, const Hsp& hsp);

    protected:
        int             num_;
        double          bit_score_;
        int             score_;
        double          evalue_;
        int             query_from_;
        int             query_to_;
        int             hit_from_;
        int             hit_to_;
        int             query_frame_;
        int             hit_frame_;
        int             identity_;
        int             positive_;
        int             gaps_;
        int             align_len_;
        std::string     qseq_;
        std::string     hseq_;
        std::string     midline_;
        int             count_;         // primary key
        int             hit_id_;        // foreign key
        int             query_id_;      // foreign key
};



class BlastHit
{
    public:
        typedef Table<BlastHit>     HitTable;
        typedef Column<BlastHit>    HitColumn;

        // Default constructor for Hits
        BlastHit()
        {
            num_ = 0;
        }

        // Default destructor
        ~BlastHit() { }

        // Construct Hit with given properties
        BlastHit( const int& num,
                  const std::string& id,
                  const std::string& def,
                  const std::string& accession,
                  const int& len,
                  const std::vector<Hsp>& hsp,
                  const int& count = 0,
                  const int& queryId = 0)
            : num_(num),
              id_(id),
              def_(def),
              accession_(accession),
              len_(len),
              hsp_(hsp),
              count_(count),
              query_id_(queryId)
        {
        }

        // Getters
        int getID() const { return count_ ; }
        int getQueryID() const { return query_id_ ; }
        int getHitNum() const { return num_; }
        std::string getHitId() const { return id_; }
        std::string getHitDef() const { return def_; }
        std::string getHitAccession() const { return accession_; }
        int getHitLen() const { return len_; }
        std::vector<Hsp>& getHsp() { return hsp_; }

        // Setters
        void setID( const int& count ) { count_ = count ; }
        void setQueryID( const int& queryId ) { query_id_ = queryId ; }
        void setHitNum( const int& num ) { num_ = num ; }
        void setHitId( const std::string& id );
        void setHitDef( const std::string& def )  { def_ = def ; }
        void setHitAccession( const std::string& accession )  { accession_ = accession ; }
        void setHitLen( const unsigned int& len ) { len_ = len ; }
        void setHsp( const std::vector<Hsp>& hsp )  { hsp_ = hsp ; }

        // SQL Table
        static HitTable& table() {
        static HitTable tbl = HitTable::table("hit",
                HitColumn("query_id",   makeAttr(&BlastHit::query_id_)),
                HitColumn("hit_id",     makeAttr(&BlastHit::count_)),
                HitColumn("hit_num",    makeAttr(&BlastHit::num_)),
                HitColumn("gene_id",    makeAttr(&BlastHit::id_)),
                HitColumn("accession",  makeAttr(&BlastHit::accession_)),
                HitColumn("definition", makeAttr(&BlastHit::def_)),
                HitColumn("length",     makeAttr(&BlastHit::len_))
            );
            return tbl;
        }

        // operators
        friend std::ostream& operator<<(std::ostream& out, const BlastHit& hit);

    protected:
        int                 num_;       // hit_num
        std::string         id_;        // gene_id
        std::string         def_;       // definition
        std::string         accession_; // accession
        int                 len_;       // length
        std::vector<Hsp>    hsp_;
        int                 count_;     // hit_id; primary key
        int                 query_id_;  // query_id; foreign key
};


class BlastQuery
{
    public:
        typedef Table<BlastQuery>   QueryTable;
        typedef Column<BlastQuery>  QueryColumn;

        // Default constructor for BLAST Queries
        BlastQuery() { }

        // Default destructor
        ~BlastQuery() { }

        // Construct Query with given properties
        BlastQuery( const unsigned int& id,
                    const std::string& def,
                    const unsigned int& len,
                    const std::vector<BlastHit>& hit )
            : id_(id),
              def_(def),
              len_(len),
              hit_(hit)
        {
            // cout << "Constructed query " << this << endl;
        }

        // Getters
        int getQueryId() const { return id_; }
        std::string getQueryDef() const { return def_; }
        int getQueryLen() const { return len_; }
        std::vector<BlastHit>& getHit() { return hit_; }

        // Setters
        void setQueryId( const int& id )  { id_ = id ; }
        void setQueryDef( const std::string& def )  { def_ = def ; }
        void setQueryLen( const int& len ) { len_ = len ; }
        void setHit( const std::vector<BlastHit>& hit )  { hit_ = hit ; }

        // Table
        static QueryTable& table() {
            static QueryTable tbl = QueryTable::table("query",
                QueryColumn("query_id",   makeAttr(&BlastQuery::id_)),
                QueryColumn("query_def",  makeAttr(&BlastQuery::def_)),
                QueryColumn("query_len",  makeAttr(&BlastQuery::len_))
            );
            return tbl;
        }

        // operators
        friend std::ostream& operator<<(std::ostream& out, const BlastQuery& query);

    private:
        int                     id_;
        std::string             def_;
        int                     len_;
        std::vector<BlastHit>   hit_;
};

#endif // BLAST_HPP
