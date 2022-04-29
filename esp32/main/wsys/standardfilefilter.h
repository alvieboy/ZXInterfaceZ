#pragma once

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
