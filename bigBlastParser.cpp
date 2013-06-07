#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <sys/stat.h>
#include <stdexcept>

#include "BlastSAXHandler.hpp"

using namespace xercesc;
using std::cerr;
using std::cout;
using std::endl;

static void show_usage(std::string name);
std::string replace_extension(std::string, const std::string);
bool file_exists(std::string&);

// set defaults
std::string xmlFile;                // must be provided
std::string dbName("");
int max_hit = 20;
int max_hsp = 20;
int reset_at = 1000;
int checkFileName;
char* offset;

//
// only for debugging command line args
//
//static void show_args();


int main(int argc, char *argv[])
{
    if (argc < 2) {
        show_usage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            show_usage(argv[0]);
            return 0;
        } else if (i + 1 != argc) {
            if (arg == "-o" || arg == "--out") {
                dbName = argv[++i];
            } else if (arg == "--max_hit" ) {
                max_hit = strtol( argv[++i], &offset, 10 );
            } else if (arg == "--max_hsp" ) {
                max_hsp = strtol( argv[++i], &offset, 10 );
            } else if (arg == "--reset_at" ) {
                reset_at = strtol( argv[++i], &offset, 10 );
            }
        } else {
            xmlFile = argv[i];
        }
    }

    // the parsed file name must not start with '-'
    std::string::size_type idx = xmlFile.find('-');
    // and it must not start with a number
    checkFileName = strtol( xmlFile.c_str(), &offset, 10 );
    // if xmlFile is empty, does start with '-' or a number,
    // the arguments were most likely messed up.
    if (xmlFile.empty() || idx == 0 || checkFileName != 0) {
        cerr << "No valid input file provided.\n\n"
             << "USAGE:\n\t" << argv[0] << " [options] blastfile.xml\n"
             << endl;
        return 1;
    }

    if (!file_exists(xmlFile)) {
        cerr << "XML file '" << xmlFile << "' does not exist." << endl;
        return 1;
    }

    if (dbName.empty()) {
        // no dbName has been provided use xml filename as basename
        dbName = replace_extension(xmlFile, "db");
    }

    if (file_exists(dbName)) {
        // choose another name or remove the offending file
        cerr << "DB file '" << dbName << "' already exists." << endl;
        return 1;
    }

//    only for debugging command line args
//    show_args();
//    return 0;

    try {
        XMLPlatformUtils::Initialize();
    } catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage ());
        cout    << "Error during intialisation! :\n"
                << message << endl;
        XMLString::release(&message);
        return 1;
    }

    try {
        // optain parser and register the Blast Query Handler
        std::unique_ptr<SAX2XMLReader> parser{ XMLReaderFactory::createXMLReader() };
        std::vector<BlastQuery> queryList;
        BlastQueryContentHandler queryHandler(queryList, dbName, max_hit,
                                              max_hsp, reset_at);
        parser->setContentHandler(&queryHandler);
        parser->setErrorHandler(&queryHandler);
        // parse xmlFile
        parser->parse( xmlFile.c_str() );

    } catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage ());
        cout << "Exception message is: \n"
             << message << endl;
        XMLString::release(&message);
        return -1;
    }
    catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage ());
        cout << "Exception message is: \n"
             << message << endl;
        XMLString::release(&message);
        return -1;
    }
    catch (...) {
        cout << "Unexpected exception" << endl;
        return -1;
    }

    XMLPlatformUtils::Terminate();
    return 0;
}

static void show_usage(std::string name) {
    cerr << "USAGE:\n\t" << name << " [options] <blastfile>.xml\n"
         << "OPTIONS:\n"
         << "\t-h,--help\t\tShow this help message\n"
         << "\t-o,--out <filename>\tPath to SQLite file. Default [<blastfile>.db].\n"
         << "\t--max_hit <n>\t\tNumber of hits parsed. Default [20] (set [-1] for all).\n"
         << "\t--max_hsp <n>\t\tNumber of hsps parsed. Default [20] (set [-1] for all).\n"
         << "\t--reset_at <n>\t\tAfter <n> parsed queries the data is dumped to"
         << " the SQLite DB.\n\t\t\t\tDefault [1000].\n"
         << "\t<blastfile.xml> Input file.\n"
         << "DESCRIPTION\n"
         << "\tblastParse 0.1-alpha -- Convert XML Blast Reports to an SQLite DB\n\n"
         << endl;
}

std::string replace_extension( std::string filename,
                               const std::string extension )
{
    std::string tmpname, extname, basename;

    // find period in filename
    std::string::size_type idx = filename.find('.');
    if (idx == std::string::npos) {
        // filename does not contain any period
        tmpname = filename + '.' + extension;
    }
    else {
        // split filename into base name and extension
        basename = filename.substr(0, idx);
        extname = filename.substr(idx + 1);
        if (extname.empty()) {
            // contains period but no extension: append tmp
            tmpname = filename;
            tmpname += extension;
        }
        else if (extname == extension) {
            // do nothing
            tmpname = filename;
        }
        else {
            // replace any extension with tmp
            tmpname = filename;
            tmpname.replace(idx + 1, std::string::npos, extension);
        }
    }
    return tmpname;
}

inline bool file_exists( std::string& name ) {
    struct stat file;
    return( stat( name.c_str(), &file ) == 0 );
}

//
// only for debugging command line args
//
//static void show_args() {
//    cout << "xmlFile: " << xmlFile
//         << "\ndbName: " << dbName
//         << "\nmax_hit: " << max_hit
//         << "\nmax_hsp: " << max_hsp
//         << "\nreset_at: " << reset_at
//         << "\ncheckFileName: " << checkFileName << endl;
//}


