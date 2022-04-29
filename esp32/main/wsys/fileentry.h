#pragma once

#include <string>
#include <vector>
#include <inttypes.h>

struct FileEntry {
public:
    FileEntry(): m_name("") {}
    FileEntry(uint8_t flags, const char *str): m_flags(flags), m_name(str) {}
    uint8_t m_flags;
    std::string m_name;
    const char *str() const {return m_name.c_str(); }
    uint8_t flags() const { return m_flags; }
};

typedef std::vector<FileEntry> FileEntryList;
