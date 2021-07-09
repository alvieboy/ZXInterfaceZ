# Webapp

This project was generated with [Angular CLI](https://github.com/angular/angular-cli) version 10.0.8.

## Development server

Run `ng serve` for a dev server. Navigate to `http://localhost:4200/`. The app will automatically reload if you change any of the source files.

## Code scaffolding

Run `ng generate component component-name` to generate a new component. You can also use `ng generate directive|pipe|service|class|guard|interface|enum|module`.

## Build

Run `ng build` to build the project. The build artifacts will be stored in the `dist/` directory. Use the `--prod` flag for a production build.

## Running unit tests

Run `ng test` to execute the unit tests via [Karma](https://karma-runner.github.io).

## Running end-to-end tests

Run `ng e2e` to execute the end-to-end tests via [Protractor](http://www.protractortest.org/).

## Further help

To get more help on the Angular CLI use `ng help` or go check out the [Angular CLI README](https://github.com/angular/angular-cli/blob/master/README.md).


# ZX Interface Z

To build:

```
ng build --prod
gulp default
gulp install-into-spiffs
```

## Local Development

### Prerequisites

- [NodeJS](https://nodejs.org/en/) 12.x or later (including `npm`)
- [Gulp](https://gulpjs.com/) `npm install --gloabal gulp-cli`

To install NodeJS on Debian/Ubuntu don't use the provided packages since they are for NodeJS  8.11.

To install the required NPM modules do:

```
npm install
```

### Mock Backend

To start the _mock backend_ run:

```
npm run mock-backend
```

### Run the Development Server

To start the auto reloading development using the _mock backend_ server do:

```
ng serve -o
```

To run towards the _host mode only emulator_ use the option `--proxy=proxy-emulator.conf.json`.
