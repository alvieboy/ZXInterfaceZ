#ifndef __WSYS_STANDARD_FILE_FILTER_H__
#define __WSYS_STANDARD_FILE_FILTER_H__

#include "filefilter.h"

namespace StandardFileFilter
{
    const FileFilter *AllFilesFileFilter();
    const FileFilter *AllSnapshotsFileFilter();
    const FileFilter *AllPokesFileFilter();
    const FileFilter *AllTapesFileFilter();
    const FileFilter *AllTapesAndScreensFileFilter();
    const FileFilter *TAPFileFilter();
    const FileFilter *TZXFileFilter();
};

#endif
