#include "BlastSAXHandler.hpp"

using std::cout;
using std::endl;

// static class members

BlastQuery BlastQueryContentHandler::query_ = BlastQuery();
BlastHit BlastQueryContentHandler::hit_ = BlastHit();
Hsp BlastQueryContentHandler::hsp_ = Hsp();

bool BlastQueryContentHandler::inside_query_ = false;
bool BlastQueryContentHandler::inside_hit_ = false;
bool BlastQueryContentHandler::skip_hit_ = false;
bool BlastQueryContentHandler::inside_hsp_ = false;
bool BlastQueryContentHandler::skip_hsp_ = false;

void BlastQueryContentHandler::printState()
{
    cout << "inside_query = " << inside_query_
         << "; inside_hit = " << inside_hit_
         << "; skip_hit_ = " << skip_hit_
         << "; inside_hsp = " << inside_hsp_
         << "; skip_hsp_ = " << skip_hsp_
         << endl;
}

// Class methods


void BlastQueryContentHandler::startDocument() {
    // cout << "Calling startDocument()" << endl;
}


// call dump_to_sqliteDB() to clean up after the last round of parsing
void BlastQueryContentHandler::endDocument() {
    // cout << "Calling endDocument()" << endl;
    this->dump_to_sqliteDB();
}


void BlastQueryContentHandler::startElement(
        const XMLCh * const uri,
        const XMLCh * const localname,
        const XMLCh * const qname,
        const Attributes &attrs)
{
    // cout << "Calling startElement( <" << toNative(qname) << "> )" << endl;
    if (qname == iteration)
    {
        // entering a query; set state to 'inside_query'
        inside_query_   = true;
        inside_hit_     = false;
        skip_hit_       = false;
        inside_hsp_     = false;
        skip_hsp_       = false;
        // BlastQueryContentHandler::printState ();

        // initialize new query, hit, and hsp and count query one up
        query_ = BlastQuery();
        hit_ = BlastHit();
        hsp_ = Hsp();
        query_.setID( ++queryCounter_ );
        // cout << "Parsing query number: " << queryCounter_ << endl;
    }
    else if (qname == iterationHits)
    {
        // entering a list of hits; set state to 'inside_hit'
        inside_query_   = false;
        inside_hit_     = true;
        inside_hsp_     = false;
        // BlastQueryContentHandler::printState();

        // clear out the previous 'hit_list'
        hit_list_.clear();
    }
    else if (qname == hit)
    {
        // entering an individual hit;
        // check if we want to parse all hits (max_hit ==  -1)
        // or if the current hit_num is smaller than max_hit.
        if ( max_hit_ == -1 || hit_.getHitNum () < max_hit_ )
        {
            // clean up previous hit and hsp; count one up
            hit_ = BlastHit();
            hsp_ =  Hsp();
            hit_.setID( ++hitCounter_ );
            hit_.setQueryID( query_.getID() );
            inside_hit_ = true;
            // BlastQueryContentHandler::printState();
        }
        else
        {
            // switch off hit parsing and hsp parsing
            skip_hit_ = true;
            skip_hsp_ = true;
            // BlastQueryContentHandler::printState();
        }
    }
    else if (qname == hitHsps && !skip_hit_)
    {
        // entering a list of hsps and set state to 'inside_hsp'
        inside_query_   = false;
        inside_hit_     = false;
        inside_hsp_     = true;
        // BlastQueryContentHandler::printState();

        // and clear out the previous hsp_list
        hsp_list_.clear();
    }
    else if (qname == hsp && !skip_hit_ && !skip_hsp_)
    {
        // entering an individual hsp;
        // check if we want to parse all hsps (max_hsp ==  -1)
        // or if the current hsp_num is smaller than max_hsp.
        if ( max_hsp_ == -1 || hsp_.getHspNum () < max_hsp_ )
            // clean up the previous hsp and count one up
        {
            hsp_ = Hsp();
            hsp_.setID( ++hspCounter_ );
            hsp_.setHitID( hit_.getID() );
            hsp_.setQueryID( hit_.getQueryID() );
        }
        else
        {
            // switch off hsp parsing
            skip_hsp_ = true;
            // BlastQueryContentHandler::printState();
        }
    }
    else
    {
        // If we encounter other tags we clear currText_ in
        // preparation for the characters() callback
        currText_.clear();
    }
};


void BlastQueryContentHandler::endElement(
        const XMLCh * const uri,
        const XMLCh * const localname,
        const XMLCh * const qname )
{
    // cout << "Calling endElement( </" << toNative(qname) << "> )" << endl;
    if (qname == hsp && !skip_hsp_)
    {
        // leave hsp and push it onto hsp_list
        hsp_list_.push_back(hsp_);
    }
    else if (qname == hitHsps && !skip_hit_)
    {
        // leave the last of the hsps; put hsp_list into the current hit instance
        hit_.setHsp(hsp_list_);

        // toggle off 'inside_hsp'; toggle on 'inside_hit'
        inside_hsp_ = false;
        inside_hit_ = true;
    }
    else if (qname == hit && !skip_hit_)
    {
        // leave hit and push it onto hit_list
        hit_list_.push_back(hit_);
    }
    else if (qname == iterationHits)
    {
        // leave the last of the hits; put hit_list into the current query instance
        query_.setHit(hit_list_);
    }
    else if (qname == iteration)
    {
        // when we leave the query, we push the current query onto query_list_
        query_list_.push_back(query_);
        // if we are at a multiple of 'reset_at_', we dump the 'query_list_' to SQLite
        if (query_.getID() % reset_at_ == 0)
        {
            //cout << "Reset at: " << reset_at_ << "; Parsing query number: " << queryCounter_ << endl;
            this->dump_to_sqliteDB();
        }
    }
    // for all other nodes we set the appropriate values in query, hit, or hsp
    else if (inside_query_)
    {
        if (qname == queryNum)
        {
            query_.setQueryNum (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == queryDef)
        {
            query_.setQueryDef (toNative (currText_));
        }
        else if (qname == queryLen) {
            query_.setQueryLen (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
    }
    else if (inside_hit_ && !skip_hit_)
    {
        if (qname == hitNum)
        {
            // cout << "Setting HitNum: " << toNative(currText_) << endl;
            hit_.setHitNum (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == hitId)
        {
            // cout << "Setting HitId: " << toNative(currText_) << endl;
            hit_.setHitId (toNative (currText_));
        }
        else if (qname == hitDef)
        {
            hit_.setHitDef (toNative (currText_));
        }
        else if (qname == hitAccn)
        {
            hit_.setHitAccession (toNative (currText_));
        }
        else if (qname == hitLen) {
            hit_.setHitLen (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
    }
    else if (inside_hsp_ && !skip_hit_ && !skip_hsp_)
    {
        if (qname == hspNum)
        {
            // cout << "Setting HspNum: " << toNative (currText_) << endl;
            hsp_.setHspNum (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == bitscore)
        {
            hsp_.setBitScore (static_cast<double> (std::stod (toNative (currText_))));
        }
        else if (qname == score)
        {
            hsp_.setScore (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == evalue)
        {
            hsp_.setEvalue (static_cast<double> (std::stod (toNative (currText_))));
        }
        else if (qname == queryFrom)
        {
            hsp_.setQueryFrom (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == queryTo)
        {
            hsp_.setQueryTo (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == hitFrom)
        {
            hsp_.setHitFrom (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == hitTo)
        {
            hsp_.setHitTo (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == queryFrame)
        {
            hsp_.setQueryFrame (static_cast<int> (std::stoi (toNative (currText_))));
        }
        else if (qname == hitFrame)
        {
            hsp_.setHitFrame (static_cast<int> (std::stoi (toNative (currText_))));
        }
        else if (qname == identity)
        {
            hsp_.setIdentity (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == positive)
        {
            hsp_.setPositive (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == gaps)
        {
            hsp_.setGaps (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == alignLen)
        {
            hsp_.setAlignLen (static_cast<unsigned int> (std::stoi (toNative (currText_))));
        }
        else if (qname == qseq)
        {
            hsp_.setQSeq (toNative (currText_));
        }
        else if (qname == hseq)
        {
            hsp_.setHSeq (toNative (currText_));
        }
        else if (qname == midline)
        {
            hsp_.setMidline (toNative (currText_));
        }
    }
    else
    {
    }
};


void BlastQueryContentHandler::characters( const XMLCh * const chars,
                                           const XMLSize_t length )
{
    currText_.append(chars, length);
};


void BlastQueryContentHandler::fatalError( const SAXParseException& exc )
{
    std::string message = toNative (exc.getMessage());
    std::cout << "Fatal Error: " << message
              << " at line: " << exc.getLineNumber() << std::endl;
}


// dump query, hit, and hsp lists to SQlite DB and clean up;
void BlastQueryContentHandler::dump_to_sqliteDB()
{
    try {
        hit_list_.clear();
        hsp_list_.clear();
        db_.insert<BlastQuery>(begin(query_list_), end(query_list_));
        cout << "Processed " << queryCounter_;
        for (auto& query : query_list_)
        {
            std::vector<BlastHit> hits = query.getHit();
            hit_list_.reserve( hit_list_.size() + hits.size() );
            hit_list_.insert( end(hit_list_), begin(hits), end(hits) );
        }
        query_list_.clear();
        db_.insert<BlastHit>(begin(hit_list_), end(hit_list_));
        cout << " queries, " << hitCounter_;
        for (auto& hit : hit_list_)
        {
            std::vector<Hsp> hsps = hit.getHsp();
            hsp_list_.reserve( hsp_list_.size() + hsps.size() );
            hsp_list_.insert( end(hsp_list_), begin(hsps), end(hsps) );
        }
        hit_list_.clear();
        db_.insert<Hsp>(begin(hsp_list_), end(hsp_list_));
        cout << " hits, and " << hspCounter_ << " hsps." << endl;
        hsp_list_.clear();

    } catch (const std::logic_error& toCatch) {
        cout << toCatch.what() << endl;
    }
}

