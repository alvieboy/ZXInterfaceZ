#include "standardfilefilter.h"

#define EXTENSIONS(x...) (const char *[]){ x }

static const FileFilter _allFilesFileFilter("All files", 0, NULL);
static const FileFilter _allSnapshotsFileFilter("Snapshots", 2, EXTENSIONS("sna", "z80") );
static const FileFilter _allPokesFileFilter("Pokes", 1, EXTENSIONS("pok") );
static const FileFilter _allRomsFileFilter("Roms", 1, EXTENSIONS("rom") );
static const FileFilter _allTapesFileFilter("Tapes", 2, EXTENSIONS("tap","tzx") );
static const FileFilter _TAPFileFilter("TAP Tapes", 1, EXTENSIONS("tap") );
static const FileFilter _TZXFileFilter("TZX Tapes", 1, EXTENSIONS("tzx") );

namespace StandardFileFilter
{
    const FileFilter *AllFilesFileFilter() { return &_allFilesFileFilter; }
    const FileFilter *AllSnapshotsFileFilter() { return &_allSnapshotsFileFilter; }
    const FileFilter *AllPokesFileFilter() { return &_allPokesFileFilter; }
    const FileFilter *AllTapesFileFilter() { return &_allTapesFileFilter; }
    const FileFilter *AllRomsFileFilter() { return &_allRomsFileFilter; }
    const FileFilter *TAPFileFilter() { return &_TAPFileFilter; }
    const FileFilter *TZXFileFilter() { return &_TZXFileFilter; }
};
