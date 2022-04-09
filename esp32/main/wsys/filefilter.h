#pragma once

#include <string>
#include <initializer_list>

class FileFilterBase
{
public:
    virtual const char *getDescription() const = 0;
    virtual bool match_extension(const char *filename) const = 0;
};


class FileFilter: public FileFilterBase
{
public:
    FileFilter(const char *description, unsigned count, const char *list[]): m_descr(description), m_count(count), m_ext(list)
    {
    }
    virtual const char *getDescription() const override {
        if (m_descr)
            return m_descr;
        return "All files";
    }
    virtual bool match_extension(const char *filename) const override;
private:
    const char *m_descr;
    int m_count; // -1 for no-match, 0 to all-match
    const char **m_ext;
};
