#include "Blast.hpp"

// write an Hsp to an ostream
std::ostream& operator<<(std::ostream& out, const Hsp& hsp) {
    out << "Hsp [" << hsp.num_ << "] "
        << "Score: " << hsp.bit_score_ << " bits (" << hsp.score_ << "), "
        << "Expect: " << hsp.evalue_ << ",\n"
        << "Identities: " << hsp.identity_ << ", "
        << "Positives: " << hsp.positive_ << ", "
        << "Gaps: " << hsp.gaps_ << "\n" ;
    return out;
}

// Write a Hit to an ostream
std::ostream& operator<<(std::ostream& out, const BlastHit& hit) {
    out << "Hit [" << hit.num_ << "] "
        << "Hit: " << hit.id_ << " " << hit.def_ << "\n";
    for (auto& hsp : hit.hsp_) {
        out << hsp ;
    }
    return out;
}

// Write a BLAST query to an ostream
std::ostream& operator<<(std::ostream& out, const BlastQuery& query) {
    out << "Query [" << query.id_ << "] : "
        << query.def_ << " ( " << query.len_ << " letters)\n";
    for (auto& hit : query.hit_) {
        out << hit ;
    }
    return out;
}

// split a string
void strsplit(const std::string& str, char c, std::vector<std::string>& out) {
    std::string::size_type i = 0;
    std::string::size_type j = str.find(c);
    while (j != std::string::npos) {
        out.push_back(str.substr(i, j-i));
        i = ++j;
        j = str.find(c, j);
        if (j == std::string::npos)
            out.push_back(str.substr(i, str.length( )));
    }
}


void BlastHit::setHitId( const std::string& id )  {
    std::vector<std::string> out;
    strsplit(id, '|', out);
    this->id_ = out[1];
}


