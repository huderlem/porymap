#ifndef QSTRINGHASH_H
#define QSTRINGHASH_H

#include <QHash>
#include <QString>
#include <functional>

// This is a custom hash function for QString so it can be used as
// a key in a std::hash structure. Qt 5.14 added this function, so
// this file should only be included in earlier versions.
namespace std {
  template<> struct hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
      return static_cast<size_t>(qHash(s));
    }
  };
}

#endif // QSTRINGHASH_H
