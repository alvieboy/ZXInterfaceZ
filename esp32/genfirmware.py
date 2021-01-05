import json
import argparse
import tarfile
import StringIO
import subprocess

class FirmwareBuilder():
    def __init__(self,version, compat, out):
        self.version = version
        self.compat  = compat
        self.out  = out
        self.files = []
        self.outfile = tarfile.open(out, "w")
        

    def pack(self, filename, name, type, compression):
        info = { "filename": filename, "name": name, "type": type, "compression": compression }
        self.files.append(info)

    def finish(self):
        jfiles = []
        for f in self.files:
            jfiles.append( { "name": f['name'],
                "type": f['type'], "compression": f['compression'] } )

        manifest = {"update": {
            "version": self.version,
            "compat": self.compat,
            "files" : jfiles
            }
        }

        manifestdata = json.dumps(manifest)

        manifestinfo = tarfile.TarInfo('MANIFEST.json')
        manifestinfo.size = len(manifestdata)

        self.outfile.addfile(manifestinfo, StringIO.StringIO(manifestdata))


        for f in self.files:
            if f['compression']=="rle":
                retcode = subprocess.call(["./rle", f['filename'], f['filename']+".rle"])
                self.outfile.add(f['filename']+".rle", arcname=f['name'])
            else:
                self.outfile.add(f['filename'], arcname=f['name'])


        self.outfile.close()

    

def main():
    parser = argparse.ArgumentParser(description="InterfaceZ Firmware Generator",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("--version",
                        help="version of the firmware")

    parser.add_argument("--compat",
                        help="List of compatible boards")

    parser.add_argument("--out",
                        help="File to generate")

    args = parser.parse_args()
    f = FirmwareBuilder(args.version, args.compat, args.out)
    f.pack("build/esp32_interfacez.bin", "esp32_interfacez.bin", "ota", "none")
    f.pack("build/resources.bin", "resources.bin", "resources", "rle")
    f.finish()

    return

if __name__ == "__main__":
    main()
