#ifndef XERCESSTRING_HPP
#define XERCESSTRING_HPP

#include <boost/scoped_array.hpp>
#include <xercesc/util/XMLString.hpp>

typedef std::basic_string<XMLCh> XercesString;

// convert from narrow character to wide character
inline
XercesString fromNative (const char* str) {
    boost::scoped_array<XMLCh> ptr(xercesc::XMLString::transcode (str));
    return XercesString(ptr.get ());
}

inline
XercesString fromNative (const std::string& str) {
    return fromNative (str.c_str ());
}

// convert from wide character to narrow character
inline
std::string toNative (const XMLCh* str) {
    boost::scoped_array<char> ptr(xercesc::XMLString::transcode (str));
    return std::string(ptr.get ());
}

inline
std::string toNative (const XercesString& str) {
    return toNative (str.c_str ());
}


#endif // XERCESSTRING_HPP
