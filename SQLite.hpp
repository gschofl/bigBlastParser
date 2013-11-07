#ifndef SQLITE_HPP
#define SQLITE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <memory>
#include <sqlite3.h>

using std::cout;
using std::endl;
using std::string;
using std::vector;

const std::string BLAST_DB_SCHEMA = R"(
CREATE TABLE query(
        query_id      INTEGER,
        query_num     INTEGER,
        query_def     TEXT,
        query_len     INTEGER,
        PRIMARY KEY (query_id)
        );
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
CREATE INDEX Fquery ON query (query_id);
CREATE INDEX Fhit ON hit (hit_id);
CREATE INDEX Fhit_query ON hit (query_id);
CREATE INDEX Fhit_hit_query ON hit (query_id, hit_id);
CREATE INDEX Fhsp ON hsp (hsp_id);
CREATE INDEX Fhsp_query ON hsp (query_id);
CREATE INDEX Fhsp_hit ON hsp (hit_id);
CREATE INDEX Fhsp_hit_query ON hsp (query_id, hit_id, hsp_id);
)";

typedef void(*del)(void*);

vector<string> split_string(const string&, const string&, bool);

template <typename T>
class Attribute
{
public:
    Attribute(unsigned char type) : type_(type) {
    }
    virtual int getInteger(T&) const {
        throw std::logic_error("getInteger not implemented");
    }
    virtual double getFloat(T&) const {
        throw std::logic_error("getFloat not implemented");
    }
    virtual std::string getText(T&) const {
        throw std::logic_error("getText not implemented");
    }
    virtual void setInteger(T&, int) {
        throw std::logic_error("setInteger not implemented");
    }
    virtual void setFloat(T&, double) {
        throw std::logic_error("setFloat not implemented");
    }
    virtual void setText(T&, const std::string&) {
        throw std::logic_error("setText not implemented");
    }

    unsigned char type_;
};


template <typename T>
class TextAttribute : public Attribute<T>
{
public:
    TextAttribute(string T::* text) : Attribute<T>(SQLITE_TEXT), text_(text) {
    }
    string getText(T& t) const {
        return t.*text_;
    }
    void setText(T& t, const string& text) {
        t.*text_ = text;
    }
protected:
    string T::* text_;
};


template <typename T>
class IntegerAttribute : public Attribute<T>
{
public:
    IntegerAttribute(int T::* integer) : Attribute<T>(SQLITE_INTEGER), integer_(integer) {
    }
    int getInteger(T& t) const {
        return t.*integer_;
    }
    void setInteger(T& t, int integer) {
        t.*integer_ = integer;
    }
protected:
    int T::* integer_;
};


template <typename T>
class FloatAttribute : public Attribute<T>
{
public:
    FloatAttribute(double T::* flt) : Attribute<T>(SQLITE_FLOAT), flt_(flt) {
    }
    double getFloat(T& t) const {
        return t.*flt_;
    }
    void setFloat(T& t, double flt) {
        t.*flt_ = flt;
    }
protected:
    double T::* flt_;
};


template<typename S, typename T>
std::shared_ptr<Attribute<T>>
makeAttr(S T::* i, typename std::enable_if<std::is_integral<S>::value, S>::type = 0) {
    return std::make_shared<IntegerAttribute<T> >(i);
}


template<typename S, typename T>
std::shared_ptr<Attribute<T>>
makeAttr(S T::* i, typename std::enable_if<std::is_floating_point<S>::value, S>::type = 0) {
    return std::make_shared<FloatAttribute<T> >(i);
}


template<typename S, typename T>
std::shared_ptr<Attribute<T>>
makeAttr(S T::* i, typename std::enable_if<std::is_same<S, std::string>::value, S>::type = "") {
    return std::make_shared<TextAttribute<T> >(i);
}


template<typename S, typename T>
std::shared_ptr<Attribute<T>>
makeAttr(S T::* i, size_t size, del d) {
    return std::make_shared<TextAttribute<T> >(i);
}


template<typename T>
class Column
{
public:
    Column(const string& name, std::shared_ptr<Attribute<T>> attr) : name_(name), attr_(attr) {}
    string name_;
    std::shared_ptr<Attribute<T>> attr_;
};


template<typename T>
class Table
{
public:
    template <typename S>
    static void storeColumn(Table& t, S col) {
        t.column_.push_back(col);
    }

    template <typename S, typename... Col>
    static void storeColumn(Table& t, S col, Col... c) {
        t.column_.push_back(col);
        storeColumn(t, c...);
    }

    template <typename... Col>
    static Table<T> table(const string& n, Col... cs) {
        Table<T> tbl;
        tbl.name_ = n;
        storeColumn(tbl, cs...);
        return tbl;
    }

    string name_;
    vector<Column<T> > column_;
};


class SqliteDB
{
public:
    template<typename T>
    class Iterator {
    public:
        typedef Iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        Iterator() : resultCode(SQLITE_DONE) {}

        Iterator(int rsc, sqlite3_stmt* s) : resultCode(rsc), stmt(s)
        {
            if (resultCode == SQLITE_OK) {
                resultCode = sqlite3_step(stmt);
                if (resultCode == SQLITE_ROW) {
                    buildObj();
                } else {
                    sqlite3_finalize(stmt);
                }
            } else {
                sqlite3_finalize(stmt);
            }
        }

        inline void buildObj() {
            int ncol = sqlite3_column_count(stmt);
            auto tbl = T::table();
            for (int i = 0; i < ncol; ++i)
            {
                string colname = reinterpret_cast<const char*>
                        (sqlite3_column_name(stmt, i));
                for (auto col : tbl.column_ )
                {
                    if (colname == col.name_) {
                        switch (col.attr_->type_) {
                        case SQLITE_INTEGER: {
                            int Integer = sqlite3_column_int(stmt, i);
                            col.attr_->setInteger(it, Integer);
                            break;
                        }
                        case SQLITE_FLOAT: {
                            double Float = sqlite3_column_double(stmt, i);
                            col.attr_->setFloat(it, Float);
                            break;
                        }
                        case SQLITE_TEXT: {
                            const string& Text =
                                    reinterpret_cast<const char*>(
                                        sqlite3_column_text(stmt, i)
                                        );
                            col.attr_->setText(it, Text);
                            break;
                        }
                        default: {
                            std::stringstream typeStr;
                            typeStr << "No case for " << col.attr_->type_;
                            throw std::logic_error(typeStr.str());
                        }
                        }
                        break;
                    }
                }
            }
        }

        self_type operator++() {
            self_type copy = *this;
            if (resultCode == SQLITE_ROW) {
                resultCode = sqlite3_step(stmt);
                if (resultCode == SQLITE_ROW && resultCode != SQLITE_ERROR
                        && resultCode != SQLITE_DONE) {
                    buildObj();
                }
                return copy;
            } else {
                sqlite3_finalize(stmt);
                return Iterator();
            }
        }

        reference operator*() { return it; }
        pointer operator->() { return &it; }

        bool operator==(const self_type& rhs) {
            if (resultCode != rhs.resultCode) {
                return false;
            } else if (resultCode == rhs.resultCode
                       && resultCode == SQLITE_ROW) {
                int tCnt = sqlite3_column_count(stmt);
                int oCnt = sqlite3_column_count(rhs.stmt);

                if (tCnt != oCnt) {
                    return false;
                }

                // if the column entries are different the it is different
                for (int it = 0; it < tCnt; ++it)
                {
                    string tc = reinterpret_cast<const char*>(
                                sqlite3_column_text(stmt, it)
                                );
                    string oc = reinterpret_cast<const char*>(
                                sqlite3_column_text(rhs.stmt, it)
                                );
                    if (tc != oc) {
                        return false;
                    }
                }
                return true;
            } else if (resultCode == rhs.resultCode) {
                return true;
            } else {
                return false;
            }
        }

        bool operator!=(const self_type& rhs) { return !(*this == rhs); }

    private:
        T it;
        int resultCode;
        sqlite3_stmt* stmt;
    };

    // Default constructor
    SqliteDB()
    {
        //cout << "SqliteDB default constructed: " << this << endl;
        dbName_ = "";
        db_ = nullptr;
    }

    // Open an existing database with read/write access
    SqliteDB(const string& dbName) : dbName_(dbName), db_(nullptr)
    {
        int rc;
        rc = sqlite3_open_v2(dbName.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, nullptr);
        //cout << "SqliteDB: \"" << dbName << "\" opened: " << this;
        //cout << "; RC: "<< rc << "; Msg: " << sqlite3_errmsg(db_) << endl;
        if (rc) {
            throw std::logic_error(string("Cannot open database ") + dbName +
                                   " because of " + sqlite3_errmsg(db_));
        }
        sqlite3_busy_timeout(db_, 1000);
        char* errorMessage;
        string sql{"PRAGMA locking_mode = EXCLUSIVE; BEGIN EXCLUSIVE; COMMIT;"};
        do {
            rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMessage);
            //cout << "Try locking: " << this;
            //cout << "; RC: "<< rc << "; Msg: " << sqlite3_errmsg(db_) << endl;
        } while (rc == SQLITE_BUSY);
        if (rc != SQLITE_OK) {
            if (errorMessage != nullptr) {
                throw std::logic_error(string("SQL error ") + errorMessage);
            }
        }
        //cout << "Db locked: " << this << endl;
    }

    //Create a new database applying schema
    SqliteDB(const string& dbName, const string& schema)
        : dbName_(dbName), db_(nullptr)
    {
        int rc;
        rc = sqlite3_open_v2(dbName.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        //cout << "SqliteDB: \"" << dbName << "\" constructed: " << this;
        //cout << "; RC: "<< rc << "; Msg: " << sqlite3_errmsg(db_) << endl;
        if (rc) {
            throw std::logic_error(string("Can't open database ") + dbName +
                                   " because of " + sqlite3_errmsg(db_));
        }
        char* errorMessage;
        string sql{"PRAGMA locking_mode = EXCLUSIVE; BEGIN EXCLUSIVE; COMMIT;"};
        rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMessage);
        if (rc != SQLITE_OK) {
            if (errorMessage != nullptr) {
                throw std::logic_error(string("SQL error ") + errorMessage);
            }
        }
        vector<string> statementStrings = split_string(schema, ";\n", false);
        for (auto& stmtStr : statementStrings)
        {
            sqlite3_stmt* stmt = nullptr;
            int state = sqlite3_prepare_v2(db_, stmtStr.c_str(), stmtStr.size(), &stmt, nullptr);
            if (state != SQLITE_OK) {
                throw std::logic_error(string("Statement: \"") + stmtStr + "\" failed with error:\"" +
                                       sqlite3_errmsg(db_) + "\"");
            }
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::logic_error(string("Statment:\"") + stmtStr + "\" failed with error:\"" +
                                       sqlite3_errmsg(db_) + "\"");
            }
        }
        //cout << "Db locked: " << this << endl;

    }

    ~SqliteDB() {
        sqlite3_close(db_);
        //cout << "SqliteDB destructed: " << this << endl;
    }

    template<typename S>
    inline static const string prepareStatment() {
        //cout << "Entering \"prepareStatment()\"" << endl;
        const Table<S>& tbl = S::table();
        std::stringstream statementString;
        statementString << "INSERT INTO ";
        statementString << tbl.name_ << "(";
        size_t ncol = tbl.column_.size() - 1;
        for (size_t i = 0; i < ncol; ++i) {
            statementString << tbl.column_[i].name_ << ',';
        }
        statementString << tbl.column_[ncol].name_ << ") Values(";
        size_t i = 1;
        for (; i <= ncol; ++i) {
            statementString << '?' << i << ',';
        }
        statementString << '?' << i << ");";
        return statementString.str();
    }

    inline void step(sqlite3_stmt* stmt, const string& statementString) {
        if(sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::logic_error(string("Insert Statment: \"") + statementString +
                                   "\" failed with error: \"" + sqlite3_errmsg(db_) + "\"");
        }
    }

    template<typename S, typename It>
    inline bool insert(It start, It end) {
        //cout << "Entering \"insert(It start, It end)\"" << endl;
        Table<S>& tbl = S::table();
        const string statementString(prepareStatment<S>());
        sqlite3_stmt* stmt;
        char* errorMessage;
        //cout << "BEGIN TRANSACTION" << endl;
        sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, &errorMessage);
        sqlite3_prepare_v2(db_, statementString.c_str(), statementString.size(), &stmt, nullptr);
        std::for_each(start, end, [this, &stmt, &statementString, &tbl] (S& it) {
            addParameter(it, tbl, stmt);
            step(stmt, statementString);
            sqlite3_reset(stmt);
        });
        sqlite3_exec(db_, "COMMIT TRANSACTION", nullptr, nullptr, &errorMessage);
        //cout << "COMMIT TRANSACTION" << std::endl;
        if (sqlite3_finalize(stmt) != SQLITE_OK) {
            throw std::logic_error(string("Insert Statment: \"") + statementString +
                                   "\" failed with error: \"" + sqlite3_errmsg(db_) + "\"");
        }

        return true;
    }

    inline unsigned int max_row(const string& what, const string& table) {
        //cout << "Entering \"max_row(const std::string& what, const string& table)\"" << endl;
        string statementString("SELECT max(" + what + ") FROM " + table + ";");
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, statementString.c_str(), -1, &stmt, nullptr);
        if (rc == SQLITE_ERROR) {
            throw std::logic_error(string("Select statement: \"") + statementString +
                                   "\" failed with error: \"" + sqlite3_errmsg(db_) + "\"");
        }
        sqlite3_step(stmt);
        int maxrow = sqlite3_column_int(stmt, 0);
        //cout << "Maximum " << table << " id: " << maxrow << endl;
        sqlite3_finalize(stmt);
        return maxrow;
    }

private:
    template<typename S>
    inline static void addParameter(S& t, Table<S>& tbl, sqlite3_stmt* stmt) {
        int i = 1;
        std::for_each(tbl.column_.begin(), tbl.column_.end(), [&t, &i, &stmt]
                      (const Column<S>& col)
        {
            switch(col.attr_->type_) {
            case SQLITE_INTEGER: {
                sqlite3_bind_int(stmt, i++, col.attr_->getInteger(t));
                break;
            }
            case SQLITE_FLOAT: {
                sqlite3_bind_double(stmt, i++, col.attr_->getFloat(t));
                break;
            }
            case SQLITE_TEXT: {
                const std::string Text = col.attr_->getText(t);
                sqlite3_bind_text(stmt, i++, Text.c_str(), Text.size(), SQLITE_STATIC);
                break;
            }
            default: {
                std::stringstream typeStr;
                typeStr << "No case for " << col.attr_->type_;
                throw std::logic_error(typeStr.str());
            }
            }
        });
    }

    string dbName_;
    sqlite3* db_;

};


#endif // SQLITE_HPP
