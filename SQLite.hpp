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

const std::string BLAST_DB_SCHEMA = R"(
CREATE TABLE query(
        query_id      INTEGER,
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

std::vector<std::string> split_string(const std::string&, const std::string&, bool);

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
        TextAttribute(std::string T::* text) : Attribute<T>(SQLITE_TEXT),
            text_(text) {
        }
        std::string getText(T& t) const {
            return t.*text_;
        }
        void setText(T& t,const std::string& text) {
            t.*text_ = text;
        }
    protected:
        std::string T::* text_;
};


template <typename T>
class IntegerAttribute : public Attribute<T>
{
    public:
        IntegerAttribute(int T::* integer) : Attribute<T>(SQLITE_INTEGER),
            integer_(integer) {
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
        FloatAttribute(double T::* flt) : Attribute<T>(SQLITE_FLOAT),
            flt_(flt) {
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
std::shared_ptr<Attribute<T> >
makeAttr(S T::* i, typename std::enable_if<std::is_integral<S>::value, S>::type = 0) {
    return std::make_shared<IntegerAttribute<T> >(i);
}


template<typename S, typename T>
std::shared_ptr<Attribute<T> >
makeAttr(S T::* i, typename std::enable_if<std::is_floating_point<S>::value, S>::type = 0) {
    return std::make_shared<FloatAttribute<T> >(i);
}


template<typename S, typename T>
std::shared_ptr<Attribute<T> >
makeAttr(S T::* i, typename std::enable_if<std::is_same<S, std::string>::value, S>::type = "") {
    return std::make_shared<TextAttribute<T> >(i);
}


template<typename S, typename T>
std::shared_ptr<Attribute<T> >
makeAttr(S T::* i, size_t size, del d) {
    return std::make_shared<TextAttribute<T> >(i);
}


template<typename T>
class Column
{
    public:
        Column(const std::string& name, std::shared_ptr<Attribute<T> > attr)
            : name_(name), attr_(attr) {
        }

        std::string name_;
        std::shared_ptr<Attribute<T> > attr_;
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
        static Table<T> table(const std::string& n, Col... cs) {
            Table<T> tbl;
            tbl.name_ = n;
            storeColumn(tbl, cs...);
            return tbl;
        }

        std::string name_;
        std::vector<Column<T> > column_;
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
                    const Table<T>& tbl = T::table();
                    for (int i = 0; i < ncol; ++i)
                    {
                        std::string colname = reinterpret_cast<const char*>
                                              (sqlite3_column_name(stmt, i));
                        for (auto& col : tbl.column_ )
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
                                        const std::string& Text =
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
                            std::string tc = reinterpret_cast<const char*>(
                                                 sqlite3_column_text(stmt, it)
                                                 );
                            std::string oc = reinterpret_cast<const char*>(
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
            // std::cout << "SqliteDB default constructed: " << this << std::endl;
            dbName_ = "";
            db_ = nullptr;
        }

        // Open an existing database with read/write access
        SqliteDB(const std::string& dbName) : dbName_(dbName), db_(nullptr)
        {
            // std::cout << "SqliteDB constructed [1]: " << this << std::endl;
            int rc;
            rc = sqlite3_initialize();
            if (rc) {
                throw std::logic_error(std::string("Cannot initialize the SQLite library"));
            }
            rc = sqlite3_open_v2(dbName.c_str(), &db_, SQLITE_OPEN_READWRITE, nullptr);
            if (rc) {
                throw std::logic_error(std::string("Cannot open database ") +
                                       dbName + " because of " + sqlite3_errmsg(db_));
            }
        }

        // Create a new database applying schema
        SqliteDB(const std::string& dbName, const std::string& schema)
            : dbName_(dbName), db_(nullptr)
        {
            // std::cout << "SqliteDB constructed [2]: " << this << std::endl;
            int rc;
            rc = sqlite3_initialize ();
            if (rc) {
                throw std::logic_error(std::string("Cannot initialize the SQLite library"));
            }
            rc = sqlite3_open_v2(dbName.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
            if (rc) {
                throw std::logic_error(std::string("Can't open database ") +
                                       dbName + " because of " + sqlite3_errmsg(db_));
            }
            std::vector<std::string> statementStrings = split_string(schema, ";\n", false);
            for (auto& stmtStr : statementStrings)
            {
                // std::cout << ">>>>" << stmtStr << "<<<<" << std::endl;
                sqlite3_stmt* stmt = nullptr;
                int state = sqlite3_prepare_v2(db_, stmtStr.c_str(), stmtStr.size(), &stmt, nullptr);
                if (state != SQLITE_OK) {
                    throw std::logic_error(std::string("Statement:\"") +
                                           stmtStr + "\" failed with error:\"" +
                                           sqlite3_errmsg(db_) + "\"");
                }
                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    throw std::logic_error(std::string("Statment:\"") +
                                           stmtStr + "\" failed with error:\"" +
                                           sqlite3_errmsg(db_) + "\"");
                }
            }
        }

        ~SqliteDB() {
            // std::cout << "SqliteDB destructed: " << this << std::endl;
            sqlite3_close(db_);
        }

        template<typename S>
        inline static const std::string prepareStatment() {
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

        inline void step(sqlite3_stmt* stmt, const std::string& statementString) {
            if(sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::logic_error(std::string("Insert Statment:\"") +
                                       statementString + "\" failed with error:\"" +
                                       sqlite3_errmsg(db_) + "\"");
            }
        }

        template<typename S, typename It>
        inline bool insert(It start, It end) {
            Table<S>& tbl = S::table();
            const std::string statementString(prepareStatment<S>());
            sqlite3_stmt* stmt;
            char* errorMessage;
            //std::cout << "BEGIN TRANSACTION" << std::endl;
            sqlite3_exec(db_, "BEGIN TRANSACTION", NULL, NULL, &errorMessage);
            sqlite3_prepare_v2(db_, statementString.c_str(), statementString.size(), &stmt, NULL);
            std::for_each(start, end, [this, &stmt, &statementString, &tbl] (S& it) {
                addParameter(it, tbl, stmt);
                step(stmt, statementString);
                sqlite3_reset(stmt);
            });
            sqlite3_exec(db_, "COMMIT TRANSACTION", NULL, NULL, &errorMessage);
            //std::cout << "COMMIT TRANSACTION" << std::endl;
            if (sqlite3_finalize(stmt) != SQLITE_OK) {
                throw std::logic_error(std::string("Insert Statment:\"") +
                                       statementString + "\" failed with error:\"" +
                                       sqlite3_errmsg(db_) + "\"");
            }

            return true;
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

        std::string dbName_;
        sqlite3* db_;

};


#endif // SQLITE_HPP
