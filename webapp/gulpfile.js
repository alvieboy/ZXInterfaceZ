const gulp = require("gulp");
const filter = require("gulp-filter");
const purify = require("gulp-purify-css");
const gzip = require("gulp-gzip");
const clean = require("gulp-clean");
const rename = require("gulp-rename");
const { series, parallel } = require("gulp");
const modifyFile = require("gulp-modify-file");

gulp.task("clean-gzip-dirs", () => {
    return gulp
        .src(["./dist/webapp-gzip/*", "./dist/webapp-only-gzip/*"])
        .pipe(clean({ force: true, allowEmpty: true }));
});

// Shorten names to < 32 chars to fit into SPIFFS
gulp.task("copy-with-shorten-file-names", () => {
    return gulp
        .src("./dist/webapp/*")
        .pipe(filter(["**/*.css", "**/*.js"]))
        .pipe(
            rename(path => {
                return {
                    dirname: path.dirname,
                    basename: path.basename.replace(/([^.]+\..{6}).*/g, '$1'),
                    extname: path.extname
                }
            })
        )
        .pipe(gulp.dest("./dist/webapp-gzip/"));
});

// #1 | Optimize CSS
/*
  Steps:
  Filter the CSS files to be optimized
  Based on their usage in Build JS files
  & Store the optimize file in the given location
*/
gulp.task("css", () => {
    return gulp
        .src("./dist/webapp-gzip/*")
        .pipe(
            filter([
              /*
              glob pattern for CSS files,
              point to files generated post angular prod build
              */
                "**/styles.*.css",
            ])
        )
        .pipe(
             /*
              glob pattern for JS files, to look for the styles usage
              the styles will be filtered based on the usage in the below files.
              Pointing to JS build output of Angular prod build
              */
            purify(["./dist/webapp-gzip/*.js"], {
                info: true,
                minify: true,
                rejected: true,
                whitelist: []
            })
        )
        .pipe(gulp.dest("./dist/webapp-gzip/"));/* Optimized file output location */
});

// # 2 | Genereate GZIP files
/*
Steps:
Read the optimized CSS in the Step #1
Apply gzip compression
*/
gulp.task("css-gzip", () => {
    return gulp
        .src("./dist/webapp-gzip/*")
        .pipe(filter(["**/*.css", "!**/*.gz"]))
        .pipe(gzip({ append: false }))
        .pipe(
            rename(path => {
                path.extname = path.extname + ".gz";
            })
        )
        .pipe(gulp.dest("./dist/webapp-gzip/"));
});

gulp.task("js-gzip", () => {
    return gulp
        .src("./dist/webapp-gzip/*")
        .pipe(filter(["**/*.js", "!**/*.gz"]))
        .pipe(gzip({ append: false }))
        .pipe(
            rename(path => {
                path.extname = path.extname + ".gz";
            })
        )
        .pipe(gulp.dest("./dist/webapp-gzip/"));
});

// # 4 | Clear ng-build CSS
/*
Delete style output of Angular prod build
*/
gulp.task("clear-ng-css", () => {
    return gulp
        .src("./dist/webapp/*")
        .pipe(filter(["**/styles*.css"]))
        .pipe(clean({ force: true }));
});

gulp.task("clear-ng-js", () => {
    return gulp
        .src("./dist/webapp/*")
        .pipe(filter(["**/*.js"]))
        .pipe(clean({ force: true }));
});

// # 5 | Copy optimized CSS
/*
Once the optimization & compression is done,
Replace the files in angular build output location
*/
gulp.task("copy-op", () => {
    return gulp.src("./dist/webapp-gzip/*").pipe(gulp.dest("./dist/webapp"));
});

// #6 | Clear temp folder
gulp.task("clear-gzip", () => {
    return gulp
        .src("./dist/webapp-gzip/*", { read: false })
        .pipe(clean({ force: true }));
});

gulp.task("copy-non-gzip", () => {
    return gulp
        .src("./dist/webapp/*")
        .pipe(filter(["**/*", "!**/*.js", "!**/styles.*.css"]))
        .pipe(gulp.dest("./dist/webapp-gzip"));
});

gulp.task("update-refs", () => {
    return gulp
        .src("./dist/webapp-gzip/index.html")
        .pipe(modifyFile((content, path, file) => {
            return content
                .replace(/src="([^.]+\..{6})[^"]*\.js"/g, 'src="$1.js"')
                .replace(/href="(styles\..{6})[0-9a-h]*\.css"/g, 'href="$1.css"');
        }))
        .pipe(gulp.dest("./dist/webapp-gzip"));
});


gulp.task("copy-only-gzip", () => {
    return gulp
        .src("./dist/webapp-gzip/*")
        .pipe(filter(["**/*", "!**/styles.*.css", "!**/*.js", "!**/3rdpartylicenses.txt"]))
        .pipe(gulp.dest("./dist/webapp-only-gzip"));
});

gulp.task("clear-files-in-spiffs", () => {
    return gulp
        .src("../esp32/spiffs/*")
        .pipe(filter(["**/main.*.js.gz", "**/polyfills.*.js.gz", "**/runtime.*.js.gz", "**/styles.*.css.gz", "**/favicon.ico"]))
        .pipe(clean({ force: true }));
});

gulp.task("copy-into-spiffs", () => {
    return gulp
        .src("./dist/webapp-only-gzip/*")
        .pipe(gulp.dest("../esp32/spiffs/"));
});

exports.installIntoSpiffs = series(
  "clear-files-in-spiffs",
  "copy-into-spiffs"
);

exports.installIntoSpiffs.displayName = 'install-into-spiffs';

/*
 ### Order of Tasks ###
 * Optimize the styles generated from ng build
 * Create compressed files for optimized css
 * Clear the angular build output css
 * Copy the optimized css to ng-bundle folder
 * Clear the temp folder
 */
exports.default = series(
    "clean-gzip-dirs",
    "copy-with-shorten-file-names",
    "css",
    "css-gzip",
    "js-gzip",
    "copy-non-gzip",
    "update-refs",
    "copy-only-gzip"
);
